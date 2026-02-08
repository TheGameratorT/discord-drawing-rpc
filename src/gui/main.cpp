#include <QApplication>
#include <QMessageBox>
#include "MainWindow.h"
#include "../common/Config.h"
#include "../common/PlatformUtils.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("DiscordDrawRPC");
    app.setOrganizationName("DiscordDrawRPC");
    app.setWindowIcon(QIcon(":/icons/icon.png"));
    app.setStyle("Fusion");  // Modern look
    
    // Check if another instance is already running
    if (DiscordDrawRPC::ProcessUtils::isGuiRunning()) {
        QMessageBox::warning(
            nullptr,
            "Already Running",
            "DiscordDrawRPC is already running.",
            QMessageBox::Ok
        );
        return 0;
    }
    
    // Load configuration
    DiscordDrawRPC::Config& config = DiscordDrawRPC::Config::instance();
    if (!config.load()) {
        qWarning() << "Failed to load configuration, using defaults";
    }
    
    DiscordDrawRPC::MainWindow window;
    window.show();
    
    return app.exec();
}
