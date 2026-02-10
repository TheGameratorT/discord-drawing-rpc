#include "DiscordRPCDaemon.h"
#include "../common/Config.h"
#include "../common/PlatformUtils.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QThread>

namespace DiscordDrawRPC {

DiscordRPCDaemon::DiscordRPCDaemon(QObject* parent)
    : QObject(parent)
    , m_rpc(nullptr)
    , m_watcher(nullptr)
    , m_reconnectTimer(nullptr)
    , m_running(false)
{
}

DiscordRPCDaemon::~DiscordRPCDaemon() {
    stop();
}

void DiscordRPCDaemon::start() {
    Config& config = Config::instance();
    config.load();
    
    QString clientId = config.getValue("discord_client_id");
    if (clientId.isEmpty()) {
        qCritical() << "Error: discord_client_id is empty in config file";
        qCritical() << "Please set discord_client_id in the config file";
        QCoreApplication::exit(1);
        return;
    }
    
    m_running = true;
    
    // Write PID file
    writePidFile();
    
    qInfo() << "Discord RPC Daemon started";
    qInfo() << "PID:" << QCoreApplication::applicationPid();
    qInfo() << "State file:" << config.getStateFilePath();
    
    // Initialize Discord RPC
    m_rpc = new DiscordRPC(clientId, this);
    
    // Try to connect
    if (m_rpc->connect()) {
        qInfo() << "Connected to Discord RPC";
    } else {
        qWarning() << "Failed to connect to Discord initially, will retry";
    }
    
    // Setup reconnect timer
    m_reconnectTimer = new QTimer(this);
    connect(m_reconnectTimer, &QTimer::timeout, this, &DiscordRPCDaemon::onReconnectTimer);
    m_reconnectTimer->start(5000); // Try to reconnect every 5 seconds if disconnected
    
    // Setup file watcher
    m_watcher = new QFileSystemWatcher(this);
    QString stateFile = config.getStateFilePath();
    
    // Make sure the state file exists
    if (!QFile::exists(stateFile)) {
        QFile file(stateFile);
        if (file.open(QIODevice::WriteOnly)) {
            file.write("{}");
            file.close();
        }
    }
    
    m_watcher->addPath(stateFile);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, 
            this, &DiscordRPCDaemon::onStateFileChanged);
    
    qInfo() << "File watcher started";
    
    // Read and apply initial state
    QJsonObject initialState = readStateFile();
    if (!initialState.isEmpty()) {
        m_lastState = initialState;
        handleCommand(initialState);
        qInfo() << "Applied initial state from file";
    }
}

void DiscordRPCDaemon::stop() {
    if (!m_running) return;
    
    m_running = false;
    
    if (m_rpc) {
        m_rpc->disconnect();
        m_rpc->deleteLater();
        m_rpc = nullptr;
    }
    
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
        m_reconnectTimer->deleteLater();
        m_reconnectTimer = nullptr;
    }
    
    if (m_watcher) {
        m_watcher->deleteLater();
        m_watcher = nullptr;
    }
    
    removePidFile();
    
    qInfo() << "Daemon stopped";
}

void DiscordRPCDaemon::onStateFileChanged() {
    QJsonObject newState = readStateFile();
    
    if (newState.isEmpty()) {
        return;
    }
    
    // Check if state actually changed
    if (newState == m_lastState) {
        return;
    }
    
    m_lastState = newState;
    handleCommand(newState);
    
    // Re-add the file to the watcher (it might have been removed on some systems)
    QString stateFile = Config::instance().getStateFilePath();
    if (!m_watcher->files().contains(stateFile)) {
        m_watcher->addPath(stateFile);
    }
}

void DiscordRPCDaemon::onReconnectTimer() {
    if (!m_rpc->isConnected()) {
        qDebug() << "Attempting to reconnect to Discord...";
        m_rpc->connect();
    }
}

QJsonObject DiscordRPCDaemon::readStateFile() {
    QString stateFile = Config::instance().getStateFilePath();
    
    QFile file(stateFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return QJsonObject();
    }
    
    return doc.object();
}

void DiscordRPCDaemon::handleCommand(const QJsonObject& stateData) {
    QString command = stateData.value("command").toString();
    
    if (command == "clear") {
        qInfo() << "Clearing presence";
        if (m_rpc->isConnected()) {
            m_rpc->clearPresence();
        }
    } else if (command == "update") {
        qInfo() << "Updating presence";
        
        if (!m_rpc->isConnected()) {
            qWarning() << "Not connected to Discord, attempting to connect...";
            if (!m_rpc->connect()) {
                qWarning() << "Failed to connect to Discord";
                return;
            }
        }
        
        // Build presence object
        QJsonObject presence;
        
        QString state = stateData.value("state").toString();
        QString details = stateData.value("details").toString();
        
        if (!state.isEmpty()) {
            presence["state"] = state;
        }
        if (!details.isEmpty()) {
            presence["details"] = details;
        }
        
        // Add timestamps
        QJsonObject timestamps;
        qint64 startTime = stateData.value("start").toVariant().toLongLong();
        if (startTime > 0) {
            timestamps["start"] = startTime;
        }
        if (!timestamps.isEmpty()) {
            presence["timestamps"] = timestamps;
        }
        
        // Add assets (images)
        QJsonObject assets;
        QString largeImage = stateData.value("large_image").toString();
        QString largeText = stateData.value("large_text").toString();
        QString smallImage = stateData.value("small_image").toString();
        QString smallText = stateData.value("small_text").toString();
        
        if (!largeImage.isEmpty()) {
            assets["large_image"] = largeImage;
            if (!largeText.isEmpty()) {
                assets["large_text"] = largeText;
            }
        }
        if (!smallImage.isEmpty()) {
            assets["small_image"] = smallImage;
            if (!smallText.isEmpty()) {
                assets["small_text"] = smallText;
            }
        }
        if (!assets.isEmpty()) {
            presence["assets"] = assets;
        }
        
        // Add buttons (if any)
        QJsonArray buttons = stateData.value("buttons").toArray();
        if (!buttons.isEmpty()) {
            presence["buttons"] = buttons;
        }
        
        qDebug().noquote() << "Presence data:" << QJsonDocument(presence).toJson(QJsonDocument::Compact);
        m_rpc->updatePresence(presence);
        
    } else if (command == "quit") {
        qInfo() << "Received quit command";
        m_running = false;
        QCoreApplication::quit();
    }
}

void DiscordRPCDaemon::writePidFile() {
    QString pidFile = Config::instance().getDaemonPidFilePath();
    ProcessUtils::writePidFile(pidFile, QCoreApplication::applicationPid());
}

void DiscordRPCDaemon::removePidFile() {
    QString pidFile = Config::instance().getDaemonPidFilePath();
    QFile::remove(pidFile);
}

} // namespace DiscordDrawRPC
