#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTimer>

namespace DiscordDrawRPC {

class TrayIcon : public QObject {
    Q_OBJECT
    
public:
    explicit TrayIcon(QObject* parent = nullptr);
    ~TrayIcon();
    
    void show();
    void hide();
    
private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void showWindow();
    void startDaemon();
    void stopDaemon();
    void clearStatus();
    void updateTooltip();
    void exitApp();
    
private:
    void setupIcon();
    void createMenu();
    void storeTrayPid();
    
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_menu;
    QTimer* m_statusTimer;
};

} // namespace DiscordDrawRPC
