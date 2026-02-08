#include "PlatformUtils.h"
#include "Config.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <fstream>
#endif

namespace DiscordDrawRPC {

bool ProcessUtils::isProcessRunning(qint64 pid) {
    if (pid <= 0) return false;
    
#ifdef _WIN32
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, static_cast<DWORD>(pid));
    if (process == NULL) {
        return false;
    }
    
    DWORD exitCode;
    bool result = GetExitCodeProcess(process, &exitCode) && (exitCode == STILL_ACTIVE);
    CloseHandle(process);
    return result;
#else
    // On Unix, sending signal 0 checks if process exists
    return kill(static_cast<pid_t>(pid), 0) == 0;
#endif
}

bool ProcessUtils::killProcess(qint64 pid) {
    if (pid <= 0) return false;
    
#ifdef _WIN32
    HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    if (process == NULL) {
        return false;
    }
    
    bool result = TerminateProcess(process, 0) != 0;
    CloseHandle(process);
    return result;
#else
    return kill(static_cast<pid_t>(pid), SIGTERM) == 0;
#endif
}

qint64 ProcessUtils::readPidFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return -1;
    }
    
    QTextStream in(&file);
    QString pidStr = in.readAll().trimmed();
    file.close();
    
    bool ok;
    qint64 pid = pidStr.toLongLong(&ok);
    return ok ? pid : -1;
}

bool ProcessUtils::writePidFile(const QString& filePath, qint64 pid) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to write PID file:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    out << pid;
    file.close();
    
    return true;
}

bool ProcessUtils::isProcessRunningByName(qint64 pid, const QString& namePattern) {
    if (!isProcessRunning(pid)) {
        return false;
    }
    
#ifdef _WIN32
    // On Windows, enumerate processes and check name
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    
    bool found = false;
    if (Process32First(snapshot, &entry)) {
        do {
            if (entry.th32ProcessID == static_cast<DWORD>(pid)) {
                QString exeFile = QString::fromWCharArray(entry.szExeFile);
                found = exeFile.contains(namePattern, Qt::CaseInsensitive);
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }
    
    CloseHandle(snapshot);
    return found;
#else
    // On Linux, read /proc/[pid]/cmdline
    QString cmdlinePath = QString("/proc/%1/cmdline").arg(pid);
    std::ifstream cmdlineFile(cmdlinePath.toStdString());
    
    if (!cmdlineFile.is_open()) {
        // Process might have exited or we don't have permission
        return false;
    }
    
    std::string cmdline;
    std::getline(cmdlineFile, cmdline);
    cmdlineFile.close();
    
    QString cmdlineStr = QString::fromStdString(cmdline);
    return cmdlineStr.contains(namePattern, Qt::CaseInsensitive);
#endif
}

bool ProcessUtils::isGuiRunning() {
    QString guiPidFile = Config::instance().getGuiPidFilePath();
    qint64 pid = readPidFile(guiPidFile);
    
    if (pid <= 0) {
        return false;
    }
    
    if (!isProcessRunning(pid)) {
        QFile::remove(guiPidFile);
        return false;
    }
    
    return isProcessRunningByName(pid, "DiscordDrawRPC");
}

bool ProcessUtils::isTrayRunning() {
    QString trayPidFile = Config::instance().getTrayPidFilePath();
    qint64 pid = readPidFile(trayPidFile);
    
    if (pid <= 0) {
        return false;
    }
    
    if (!isProcessRunning(pid)) {
        QFile::remove(trayPidFile);
        return false;
    }
    
    return isProcessRunningByName(pid, "DiscordDrawRPCTray");
}

bool ProcessUtils::isDaemonRunning() {
    QString pidFile = Config::instance().getPidFilePath();
    qint64 pid = readPidFile(pidFile);
    
    if (pid <= 0) {
        return false;
    }
    
    if (!isProcessRunning(pid)) {
        QFile::remove(pidFile);
        return false;
    }
    
    return isProcessRunningByName(pid, "DiscordDrawRPCDaemon");
}

} // namespace DiscordDrawRPC
