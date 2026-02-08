#include "DiscordRPC.h"
#include <QJsonDocument>
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>
#include <QThread>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace DiscordDrawRPC {

// Discord IPC opcodes
enum OpCode {
    HANDSHAKE = 0,
    FRAME = 1,
    CLOSE = 2,
    PING = 3,
    PONG = 4
};

DiscordRPC::DiscordRPC(const QString& clientId, QObject* parent)
    : QObject(parent)
    , m_clientId(clientId)
    , m_socket(nullptr)
    , m_connected(false)
    , m_readBuffer()
{
}

DiscordRPC::~DiscordRPC() {
    disconnect();
}

QString DiscordRPC::getDiscordIpcPath(int pipeNum) {
#ifdef _WIN32
    return QString("\\\\.\\pipe\\discord-ipc-%1").arg(pipeNum);
#else
    // Try different locations where Discord might create its socket
    QStringList possiblePaths = {
        QString(qEnvironmentVariable("XDG_RUNTIME_DIR")) + QString("/discord-ipc-%1").arg(pipeNum),
        QString("/run/user/%1/discord-ipc-%2").arg(getuid()).arg(pipeNum),
        QString(qEnvironmentVariable("TMPDIR")) + QString("/discord-ipc-%1").arg(pipeNum),
        QString("/tmp/discord-ipc-%1").arg(pipeNum)
    };
    
    // Return the first path that exists
    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    
    // Default to XDG_RUNTIME_DIR or /tmp
    QString runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (!runtimeDir.isEmpty()) {
        return runtimeDir + QString("/discord-ipc-%1").arg(pipeNum);
    }
    return QString("/tmp/discord-ipc-%1").arg(pipeNum);
#endif
}

bool DiscordRPC::connect() {
    if (m_connected) {
        return true;
    }
    
    // Try connecting to different pipe numbers (0-9)
    for (int i = 0; i < 10; ++i) {
        QString ipcPath = getDiscordIpcPath(i);
        
        m_socket = new QLocalSocket(this);
        
        QObject::connect(m_socket, &QLocalSocket::connected, 
                        this, &DiscordRPC::onSocketConnected);
        QObject::connect(m_socket, &QLocalSocket::disconnected, 
                        this, &DiscordRPC::onSocketDisconnected);
        QObject::connect(m_socket, &QLocalSocket::errorOccurred, 
                        this, &DiscordRPC::onSocketError);
        QObject::connect(m_socket, &QLocalSocket::readyRead, 
                        this, &DiscordRPC::onReadyRead);
        
        m_socket->connectToServer(ipcPath);
        
        if (m_socket->waitForConnected(1000)) {
            qDebug() << "Connected to Discord IPC:" << ipcPath;
            return handshake();
        } else {
            m_socket->deleteLater();
            m_socket = nullptr;
        }
    }
    
    emit error("Failed to connect to Discord. Make sure Discord is running.");
    return false;
}

void DiscordRPC::disconnect() {
    if (m_socket) {
        m_socket->disconnectFromServer();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    m_connected = false;
    m_readBuffer.clear();
}

bool DiscordRPC::handshake() {
    QJsonObject handshakeData;
    handshakeData["v"] = 1;
    handshakeData["client_id"] = m_clientId;
    
    if (!sendFrame(OpCode::HANDSHAKE, handshakeData)) {
        return false;
    }
    
    // Wait for READY response with timeout
    int retries = 50; // 5 seconds total (50 * 100ms)
    while (!m_connected && retries > 0 && m_socket && m_socket->isOpen()) {
        m_socket->waitForReadyRead(100);
        retries--;
    }
    
    return m_connected;
}

bool DiscordRPC::sendFrame(int opcode, const QJsonObject& data) {
    if (!m_socket || !m_socket->isOpen()) {
        return false;
    }
    
    QJsonDocument doc(data);
    QByteArray payload = doc.toJson(QJsonDocument::Compact);
    
    // Discord IPC frame format:
    // [opcode: int32][length: int32][payload: json bytes]
    QByteArray frame;
    QDataStream stream(&frame, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    
    stream << static_cast<qint32>(opcode);
    stream << static_cast<qint32>(payload.size());
    frame.append(payload);
    
    qint64 written = m_socket->write(frame);
    m_socket->flush();
    
    return written == frame.size();
}

bool DiscordRPC::updatePresence(const QJsonObject& presence) {
    if (!m_connected) {
        qWarning() << "Not connected to Discord RPC";
        return false;
    }
    
    QJsonObject args;
    args["pid"] = QCoreApplication::applicationPid();
    args["activity"] = presence;
    
    QJsonObject frame;
    frame["cmd"] = "SET_ACTIVITY";
    frame["args"] = args;
    frame["nonce"] = QString::number(QDateTime::currentMSecsSinceEpoch());
    
    return sendFrame(OpCode::FRAME, frame);
}

bool DiscordRPC::clearPresence() {
    if (!m_connected) {
        return false;
    }
    
    QJsonObject args;
    args["pid"] = QCoreApplication::applicationPid();
    
    QJsonObject frame;
    frame["cmd"] = "SET_ACTIVITY";
    frame["args"] = args;
    frame["nonce"] = QString::number(QDateTime::currentMSecsSinceEpoch());
    
    return sendFrame(OpCode::FRAME, frame);
}

void DiscordRPC::onSocketConnected() {
    qDebug() << "Socket connected";
}

void DiscordRPC::onSocketDisconnected() {
    qDebug() << "Socket disconnected";
    m_connected = false;
    emit disconnected();
}

void DiscordRPC::onSocketError(QLocalSocket::LocalSocketError socketError) {
    QString errorStr = m_socket ? m_socket->errorString() : "Unknown error";
    qWarning() << "Socket error:" << socketError << errorStr;
}

void DiscordRPC::onReadyRead() {
    if (!m_socket) return;
    
    m_readBuffer.append(m_socket->readAll());
    processFrames();
}

void DiscordRPC::processFrames() {
    while (m_readBuffer.size() >= 8) {
        // Read header
        QDataStream stream(m_readBuffer);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        qint32 opcode, length;
        stream >> opcode >> length;
        
        // Check if we have the complete frame
        if (m_readBuffer.size() < 8 + length) {
            return; // Wait for more data
        }
        
        // Extract the payload
        QByteArray payload = m_readBuffer.mid(8, length);
        
        // Remove processed frame from buffer
        m_readBuffer.remove(0, 8 + length);
        
        // Process the frame
        if (opcode == OpCode::FRAME) {
            QJsonDocument doc = QJsonDocument::fromJson(payload);
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject response = doc.object();
                QString cmd = response["cmd"].toString();
                
                qDebug() << "Received Discord RPC response:" << cmd;
                
                // Handle READY response from handshake
                if (cmd == "DISPATCH") {
                    QString evt = response["evt"].toString();
                    if (evt == "READY" && !m_connected) {
                        m_connected = true;
                        qDebug() << "Discord RPC handshake complete";
                        emit connected();
                    }
                }
            }
        } else if (opcode == OpCode::CLOSE) {
            qDebug() << "Discord closed connection";
            m_connected = false;
            emit disconnected();
        } else if (opcode == OpCode::PING) {
            // Respond to ping with pong
            sendFrame(OpCode::PONG, QJsonObject());
        }
    }
}

} // namespace DiscordDrawRPC
