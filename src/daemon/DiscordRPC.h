#pragma once

#include <QObject>
#include <QLocalSocket>
#include <QString>
#include <QJsonObject>

namespace DiscordDrawRPC {

/**
 * Discord RPC client implementation
 * Communicates with Discord via IPC (named pipes on Windows, Unix sockets on Linux/Mac)
 */
class DiscordRPC : public QObject {
    Q_OBJECT
    
public:
    explicit DiscordRPC(const QString& clientId, QObject* parent = nullptr);
    ~DiscordRPC();
    
    bool connect();
    void disconnect();
    bool isConnected() const { return m_connected; }
    
    bool updatePresence(const QJsonObject& presence);
    bool clearPresence();
    
signals:
    void connected();
    void disconnected();
    void error(const QString& message);
    
private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QLocalSocket::LocalSocketError socketError);
    void onReadyRead();
    
private:
    QString getDiscordIpcPath(int pipeNum = 0);
    bool handshake();
    bool sendFrame(int opcode, const QJsonObject& data);
    void processFrames();
    
    QString m_clientId;
    QLocalSocket* m_socket;
    bool m_connected;
    QByteArray m_readBuffer;
};

} // namespace DiscordDrawRPC
