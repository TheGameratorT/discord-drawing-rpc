#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include "DiscordRPCDaemon.h"
#include "../common/Config.h"
#include "../common/PlatformUtils.h"

// Global file for logging
static QFile* g_logFile = nullptr;
static QMutex g_logMutex;

// Custom message handler to write to both console and file
void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QMutexLocker locker(&g_logMutex);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString typeStr;
    
    switch (type) {
        case QtDebugMsg:
            typeStr = "DEBUG";
            break;
        case QtInfoMsg:
            typeStr = "INFO";
            break;
        case QtWarningMsg:
            typeStr = "WARNING";
            break;
        case QtCriticalMsg:
            typeStr = "CRITICAL";
            break;
        case QtFatalMsg:
            typeStr = "FATAL";
            break;
    }
    
    QString formattedMsg = QString("[%1] [%2] %3").arg(timestamp, typeStr, msg);
    
    // Write to console
    fprintf(stderr, "%s\n", formattedMsg.toLocal8Bit().constData());
    
    // Write to log file
    if (g_logFile && g_logFile->isOpen()) {
        QTextStream stream(g_logFile);
        stream << formattedMsg << "\n";
        stream.flush();
    }
    
    if (type == QtFatalMsg) {
        abort();
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("DiscordDrawingRPC");
    app.setOrganizationName("TheGameratorT");
    
    // Check if another instance is already running
    if (DiscordDrawRPC::ProcessUtils::isDaemonRunning()) {
        qCritical() << "Discord Drawing RPC Daemon is already running.";
        return 1;
    }
    
    // Setup logging to file
    DiscordDrawRPC::Config& config = DiscordDrawRPC::Config::instance();
    if (!config.load()) {
        qWarning() << "Failed to load configuration, using defaults";
    }
    
    QString logPath = config.getLogFilePath();
    g_logFile = new QFile(logPath);
    if (g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qInstallMessageHandler(messageHandler);
        qInfo() << "===== Daemon Starting =====";
    } else {
        qWarning() << "Failed to open log file:" << logPath;
    }
    
    // Create and start daemon
    DiscordDrawRPC::DiscordRPCDaemon daemon;
    daemon.start();
    
    int result = app.exec();
    
    // Cleanup
    qInfo() << "===== Daemon Shutting Down =====";
    if (g_logFile) {
        g_logFile->close();
        delete g_logFile;
        g_logFile = nullptr;
    }
    
    return result;
}
