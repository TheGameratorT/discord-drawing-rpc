#include <QApplication>
#include <QMessageBox>
#include "TrayIcon.h"
#include "../common/Config.h"
#include "../common/PlatformUtils.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("DiscordDrawRPC");
    app.setOrganizationName("DiscordDrawRPC");
    app.setWindowIcon(QIcon(":/icons/icon.png"));
    app.setQuitOnLastWindowClosed(false);  // Keep running when window closes
    
    // Check if another instance is already running
    if (DiscordDrawRPC::ProcessUtils::isTrayRunning()) {
        QMessageBox::warning(
            nullptr,
            "Already Running",
            "DiscordDrawRPC Tray is already running.",
            QMessageBox::Ok
        );
        return 0;
    }
    
    // Load configuration
    DiscordDrawRPC::Config& config = DiscordDrawRPC::Config::instance();
    if (!config.load()) {
        qWarning() << "Failed to load configuration, using defaults";
    }
    
    DiscordDrawRPC::TrayIcon tray;
    tray.show();
    
    return app.exec();
}
