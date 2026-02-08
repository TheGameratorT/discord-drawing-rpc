#include "Common.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>

#ifdef _WIN32
#include <windows.h>
#endif

namespace DiscordDrawRPC {

PlatformDirs getPlatformDirs() {
    PlatformDirs dirs;
    
#ifdef _WIN32
    // Windows: Use APPDATA for config and LOCALAPPDATA for data
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString localAppData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    
    dirs.configDir = appData + "/discord-draw-rpc";
    dirs.dataDir = localAppData + "/discord-draw-rpc";
    
#elif defined(__APPLE__)
    // macOS: Use Application Support
    QString appSupport = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    dirs.configDir = appSupport + "/discord-draw-rpc";
    dirs.dataDir = appSupport + "/discord-draw-rpc";
    
#else
    // Linux/Unix: Use XDG Base Directory Specification
    QString configHome = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (configHome.isEmpty()) {
        configHome = QDir::homePath() + "/.config";
    }
    
    QString dataHome = qEnvironmentVariable("XDG_DATA_HOME");
    if (dataHome.isEmpty()) {
        dataHome = QDir::homePath() + "/.local/share";
    }
    
    dirs.configDir = configHome + "/discord-draw-rpc";
    dirs.dataDir = dataHome + "/discord-draw-rpc";
#endif
    
    // Ensure directories exist
    QDir().mkpath(dirs.configDir);
    QDir().mkpath(dirs.dataDir);
    
    return dirs;
}

QString getExecutableDir() {
    return QCoreApplication::applicationDirPath();
}

QString getExecutablePath(const QString& name) {
    QString exeDir = getExecutableDir();
    
#ifdef _WIN32
    // Windows: Map names to .exe files
    QMap<QString, QString> exeNames;
    exeNames["discord_rpc_daemon"] = "DiscordDrawRPCDaemon.exe";
    exeNames["discord_rpc_gui"] = "DiscordDrawRPC.exe";
    exeNames["discord_rpc_tray"] = "DiscordDrawRPCTray.exe";
    
    QString exeName = exeNames.value(name, name + ".exe");
    return exeDir + "/" + exeName;
#else
    // Unix/Linux/macOS
    QMap<QString, QString> exeNames;
    exeNames["discord_rpc_daemon"] = "DiscordDrawRPCDaemon";
    exeNames["discord_rpc_gui"] = "DiscordDrawRPC";
    exeNames["discord_rpc_tray"] = "DiscordDrawRPCTray";
    
    QString exeName = exeNames.value(name, name);
    return exeDir + "/" + exeName;
#endif
}

} // namespace DiscordDrawRPC
