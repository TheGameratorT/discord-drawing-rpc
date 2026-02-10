#pragma once

#include <QString>

namespace DiscordDrawRPC {

// Process management utilities
class ProcessUtils {
public:
    // Check if a process with given PID is running
    static bool isProcessRunning(qint64 pid);
    
    // Kill a process by PID
    static bool killProcess(qint64 pid);
    
    // Read PID from file
    static qint64 readPidFile(const QString& filePath);
    
    // Write PID to file
    static bool writePidFile(const QString& filePath, qint64 pid);
    
    // Check if a specific process (by name pattern) is running
    static bool isProcessRunningByName(qint64 pid, const QString& namePattern);
    
    // Check if GUI is running
    static bool isGuiRunning();
    
    // Check if Tray is running
    static bool isTrayRunning();
    
    // Check if Daemon is running
    static bool isDaemonRunning();
    
    // Terminate process from PID file and clean up the file
    static bool terminateProcessFromPidFile(const QString& pidFilePath);
};

} // namespace DiscordDrawRPC
