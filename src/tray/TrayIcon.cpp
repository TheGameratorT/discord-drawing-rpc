#include "TrayIcon.h"
#include "../common/Config.h"
#include "../common/Common.h"
#include "../common/PlatformUtils.h"
#include "../common/DaemonIPC.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <QApplication>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QJsonObject>
#include <QProcess>
#include <QThread>

namespace DiscordDrawRPC {

const QString DISCORD_BLUE = "#5865F2";

TrayIcon::TrayIcon(QObject* parent)
    : QObject(parent)
    , m_trayIcon(nullptr)
    , m_menu(nullptr)
    , m_statusTimer(nullptr)
{
    storeTrayPid();
    
    m_trayIcon = new QSystemTrayIcon(this);
    
    setupIcon();
    createMenu();
    
    connect(m_trayIcon, &QSystemTrayIcon::activated, 
            this, &TrayIcon::onTrayActivated);
    
    // Timer to update tooltip
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &TrayIcon::updateTooltip);
    m_statusTimer->start(5000);  // Update every 5 seconds
    updateTooltip();
}

TrayIcon::~TrayIcon() {
    if (m_statusTimer) {
        m_statusTimer->stop();
    }
    
    QString trayPidFile = Config::instance().getTrayPidFilePath();
    QFile::remove(trayPidFile);
}

void TrayIcon::storeTrayPid() {
    QString pidFile = Config::instance().getTrayPidFilePath();
    ProcessUtils::writePidFile(pidFile, QCoreApplication::applicationPid());
}

void TrayIcon::setupIcon() {
    // Use embedded resource icon
    QIcon icon(":/icons/icon.png");
    
    if (!icon.isNull()) {
        m_trayIcon->setIcon(icon);
    } else {
        // Fallback: Create a simple colored icon
        QPixmap pixmap(64, 64);
        pixmap.fill(Qt::transparent);
        
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Draw a circle with Discord blue color
        painter.setBrush(QColor(DISCORD_BLUE));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(4, 4, 56, 56);
        
        // Draw "D" in white
        painter.setPen(QColor("white"));
        QFont font("Arial", 36, QFont::Bold);
        painter.setFont(font);
        painter.drawText(pixmap.rect(), Qt::AlignCenter, "D");
        
        painter.end();
        
        m_trayIcon->setIcon(QIcon(pixmap));
    }
}

void TrayIcon::createMenu() {
    m_menu = new QMenu();
    
    // Show window action
    QAction* showAction = new QAction("Show Window", this);
    connect(showAction, &QAction::triggered, this, &TrayIcon::showWindow);
    m_menu->addAction(showAction);
    
    m_menu->addSeparator();
    
    // Daemon controls submenu
    QMenu* daemonMenu = m_menu->addMenu("Presence");
    
    QAction* startDaemonAction = new QAction("Start Presence", this);
    connect(startDaemonAction, &QAction::triggered, this, &TrayIcon::startDaemon);
    daemonMenu->addAction(startDaemonAction);
    
    QAction* stopDaemonAction = new QAction("Stop Presence", this);
    connect(stopDaemonAction, &QAction::triggered, this, &TrayIcon::stopDaemon);
    daemonMenu->addAction(stopDaemonAction);
    
    m_menu->addSeparator();
    
    // Quit action
    QAction* quitAction = new QAction("Quit", this);
    connect(quitAction, &QAction::triggered, this, &TrayIcon::exitApp);
    m_menu->addAction(quitAction);
    
    m_trayIcon->setContextMenu(m_menu);
}

void TrayIcon::show() {
    m_trayIcon->show();
}

void TrayIcon::hide() {
    m_trayIcon->hide();
}

void TrayIcon::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        showWindow();
    }
}

void TrayIcon::showWindow() {
    // Check if GUI is already running
    if (ProcessUtils::isGuiRunning()) {
        return;
    }
    
    QString guiPath = getExecutablePath(ExecutableType::Gui);
    
    QProcess* process = new QProcess(this);
    
#ifdef _WIN32
    process->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags |= CREATE_NO_WINDOW;
    });
#endif
    
    if (!process->startDetached(guiPath)) {
        m_trayIcon->showMessage(
            "Error",
            "Failed to launch GUI",
            QSystemTrayIcon::Critical,
            3000
        );
    }
}

void TrayIcon::startDaemon() {
    if (ProcessUtils::isDaemonRunning()) {
        return;
    }

    // Ensure the state file has command="update" before starting daemon
    // This prevents the daemon from immediately quitting if command was "quit"
    DaemonIPC::setUpdateCommand();

    QString daemonPath = getExecutablePath(ExecutableType::Daemon);
    
    QProcess* process = new QProcess(this);
    
#ifdef _WIN32
    process->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags |= CREATE_NO_WINDOW;
    });
#endif
    
    if (process->startDetached(daemonPath)) {
        m_trayIcon->showMessage(
            "Presence Started",
            "Discord RPC presence has been started.",
            QSystemTrayIcon::Information,
            2000
        );
        updateTooltip();
    } else {
        m_trayIcon->showMessage(
            "Error",
            "Failed to start presence",
            QSystemTrayIcon::Critical,
            3000
        );
    }
}

void TrayIcon::stopDaemon() {
    if (!ProcessUtils::isDaemonRunning()) {
        return;
    }
    
    if (DaemonIPC::sendQuitCommand()) {
        m_trayIcon->showMessage(
            "Presence Stopped",
            "Discord RPC presence has been stopped.",
            QSystemTrayIcon::Information,
            2000
        );
        updateTooltip();
    }
}

void TrayIcon::updateTooltip() {
    QString tooltip;
    
    if (ProcessUtils::isDaemonRunning()) {
        tooltip = "Discord RPC - Presence Running ✅";
    } else {
        tooltip = "Discord RPC - Presence Stopped ⏸";
    }
    
    // Add current status if available
    QJsonObject state = DaemonIPC::readCurrentState();
    if (state.value("command").toString() == "update") {
        QString details = state.value("details").toString();
        if (!details.isEmpty()) {
            tooltip += "\n" + details;
        }
    }
    
    m_trayIcon->setToolTip(tooltip);
}

void TrayIcon::exitApp() {
    if (m_statusTimer) {
        m_statusTimer->stop();
    }
    
    m_trayIcon->hide();
    
    // Gracefully stop daemon if running
    if (ProcessUtils::isDaemonRunning()) {
        DaemonIPC::sendQuitCommand();
    }
    
    // Terminate GUI process if running
    if (ProcessUtils::isGuiRunning()) {
        ProcessUtils::terminateProcessFromPidFile(Config::instance().getGuiPidFilePath());
    }
    
    // Clean up our PID file
    QString trayPidFile = Config::instance().getTrayPidFilePath();
    QFile::remove(trayPidFile);
    
    QApplication::quit();
}

} // namespace DiscordDrawRPC
