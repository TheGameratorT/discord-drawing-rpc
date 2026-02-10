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
    
    dirs.configDir = appData + "/discord-drawing-rpc";
    dirs.dataDir = localAppData + "/discord-drawing-rpc";
    
#elif defined(__APPLE__)
    // macOS: Use Application Support
    QString appSupport = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    dirs.configDir = appSupport + "/discord-drawing-rpc";
    dirs.dataDir = appSupport + "/discord-drawing-rpc";
    
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
    
    dirs.configDir = configHome + "/discord-drawing-rpc";
    dirs.dataDir = dataHome + "/discord-drawing-rpc";
#endif
    
    // Ensure directories exist
    QDir().mkpath(dirs.configDir);
    QDir().mkpath(dirs.dataDir);
    
    return dirs;
}

QString getExecutableDir() {
    return QCoreApplication::applicationDirPath();
}

QString getExecutablePath(ExecutableType type) {
    QString exeDir = getExecutableDir();
    
#ifdef _WIN32
    // Windows: Use PascalCase with .exe extension
    static const char* const winExeNames[] = {
        "DiscordDrawingRPCDaemon.exe",  // Daemon
        "DiscordDrawingRPC.exe",         // Gui
        "DiscordDrawingRPCTray.exe"      // Tray
    };
    return exeDir + "/" + winExeNames[static_cast<int>(type)];
#else
    // Unix/Linux/macOS: Use kebab-case
    static const char* const unixExeNames[] = {
        "discord-drawing-rpc-daemon",    // Daemon
        "discord-drawing-rpc",           // Gui
        "discord-drawing-rpc-tray"       // Tray
    };
    return exeDir + "/" + unixExeNames[static_cast<int>(type)];
#endif
}

} // namespace DiscordDrawRPC
