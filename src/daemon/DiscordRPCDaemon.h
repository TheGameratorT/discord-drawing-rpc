#pragma once

#include <QObject>
#include <QFileSystemWatcher>
#include <QTimer>
#include "DiscordRPC.h"

namespace DiscordDrawRPC {

class DiscordRPCDaemon : public QObject {
    Q_OBJECT
    
public:
    explicit DiscordRPCDaemon(QObject* parent = nullptr);
    ~DiscordRPCDaemon();
    
    void start();
    void stop();
    
private slots:
    void onStateFileChanged();
    void onReconnectTimer();
    
private:
    void handleCommand(const QJsonObject& stateData);
    QJsonObject readStateFile();
    void writePidFile();
    void removePidFile();
    
    DiscordRPC* m_rpc;
    QFileSystemWatcher* m_watcher;
    QTimer* m_reconnectTimer;
    bool m_running;
    QJsonObject m_lastState;
};

} // namespace DiscordDrawRPC
