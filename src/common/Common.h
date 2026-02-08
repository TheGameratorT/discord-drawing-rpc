#pragma once

#include <QString>
#include <QDir>

namespace DiscordDrawRPC {

// Platform detection
#ifdef _WIN32
    constexpr bool IS_WINDOWS = true;
#else
    constexpr bool IS_WINDOWS = false;
#endif

// Get platform-specific configuration and data directories
struct PlatformDirs {
    QString configDir;
    QString dataDir;
};

PlatformDirs getPlatformDirs();

// Get the directory containing the executable
QString getExecutableDir();

// Get path to another executable in the same directory
QString getExecutablePath(const QString& name);

} // namespace DiscordDrawRPC
