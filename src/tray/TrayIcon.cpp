#include "TrayIcon.h"
#include "../common/Config.h"
#include "../common/Common.h"
#include "../common/PlatformUtils.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <QApplication>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QDebug>

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
    
    // Quick actions
    QAction* clearStatusAction = new QAction("Clear Discord Status", this);
    connect(clearStatusAction, &QAction::triggered, this, &TrayIcon::clearStatus);
    m_menu->addAction(clearStatusAction);
    
    m_menu->addSeparator();
    
    // Exit action
    QAction* exitAction = new QAction("Exit", this);
    connect(exitAction, &QAction::triggered, this, &TrayIcon::exitApp);
    m_menu->addAction(exitAction);
    
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
    QString pidFile = Config::instance().getDaemonPidFilePath();
    qint64 pid = ProcessUtils::readPidFile(pidFile);
    
    if (pid > 0 && ProcessUtils::isProcessRunning(pid)) {
        ProcessUtils::terminateProcessFromPidFile(pidFile);
        
        m_trayIcon->showMessage(
            "Presence Stopped",
            "Discord RPC presence has been stopped.",
            QSystemTrayIcon::Information,
            2000
        );
        updateTooltip();
    }
}

void TrayIcon::clearStatus() {
    QString stateFile = Config::instance().getStateFilePath();
    
    QJsonObject stateData;
    stateData["command"] = "clear";
    
    QFile file(stateFile);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(stateData);
        file.write(doc.toJson());
        file.close();
        
        m_trayIcon->showMessage(
            "Status Cleared",
            "Discord RPC status has been cleared.",
            QSystemTrayIcon::Information,
            2000
        );
    } else {
        m_trayIcon->showMessage(
            "Error",
            "Failed to clear status",
            QSystemTrayIcon::Critical,
            3000
        );
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
    QString stateFile = Config::instance().getStateFilePath();
    QFile file(stateFile);
    
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject state = doc.object();
            if (state.value("command").toString() == "update") {
                QString details = state.value("details").toString();
                if (!details.isEmpty()) {
                    tooltip += "\n" + details;
                }
            }
        }
    }
    
    m_trayIcon->setToolTip(tooltip);
}

void TrayIcon::exitApp() {
    if (m_statusTimer) {
        m_statusTimer->stop();
    }
    
    m_trayIcon->hide();
    
    // Terminate daemon process if running
    if (ProcessUtils::isDaemonRunning()) {
        ProcessUtils::terminateProcessFromPidFile(Config::instance().getDaemonPidFilePath());
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
