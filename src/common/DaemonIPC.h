#ifndef DAEMONIPC_H
#define DAEMONIPC_H

#include <QString>
#include <QJsonObject>

namespace DiscordDrawRPC {

/**
 * @brief Inter-Process Communication interface for daemon control
 * 
 * Provides a centralized interface for all UI components to communicate
 * with the Discord RPC daemon via the state file.
 */
class DaemonIPC {
public:
    /**
     * @brief Send a quit command to the daemon
     * @return true if command was written successfully, false otherwise
     */
    static bool sendQuitCommand();
    
    /**
     * @brief Set the command field to "update" while preserving all other state data
     * This is useful before starting the daemon to ensure it doesn't quit immediately
     * @return true if command was written successfully, false otherwise
     */
    static bool setUpdateCommand();
    
    /**
     * @brief Send an update command to the daemon with new Discord status
     * @param largeImage URL of the image to display
     * @param largeText Tooltip text for the large image
     * @param details Main status text (first line)
     * @param state Secondary status text (second line)
     * @param startTimestamp Unix timestamp for elapsed time (0 to disable)
     * @return true if command was written successfully, false otherwise
     */
    static bool sendUpdateCommand(
        const QString& largeImage,
        const QString& largeText,
        const QString& details,
        const QString& state,
        qint64 startTimestamp
    );
    
    /**
     * @brief Read the current state from the daemon state file
     * @return QJsonObject containing current state, or empty object if failed
     */
    static QJsonObject readCurrentState();
    
private:
    /**
     * @brief Read existing state file and return as JSON object
     * @return Current state data, or empty object if file doesn't exist/is invalid
     */
    static QJsonObject readStateFile();
    
    /**
     * @brief Write JSON data to the state file
     * @param data JSON object to write
     * @return true if write was successful, false otherwise
     */
    static bool writeStateFile(const QJsonObject& data);
    
    /**
     * @brief Helper to change only the command field while preserving other data
     * @param command The command string to set
     * @return true if command was written successfully, false otherwise
     */
    static bool setCommand(const QString& command);
};

} // namespace DiscordDrawRPC

#endif // DAEMONIPC_H
