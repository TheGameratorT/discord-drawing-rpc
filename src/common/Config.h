#pragma once

#include <QString>
#include <QJsonObject>

namespace DiscordDrawRPC {

class Config {
public:
    static Config& instance();
    
    // Load configuration from file
    bool load();
    
    // Save configuration to file
    bool save();
    
    // Get a configuration value
    QString getValue(const QString& key) const;
    
    // Set a configuration value
    void setValue(const QString& key, const QString& value);
    
    // Get the full config object
    QJsonObject getConfig() const { return m_config; }
    
    // Set the full config object
    void setConfig(const QJsonObject& config);
    
    // Config file paths
    QString getConfigFilePath() const;
    QString getStateFilePath() const;
    QString getDaemonPidFilePath() const;
    QString getGuiPidFilePath() const;
    QString getTrayPidFilePath() const;
    QString getCacheFilePath() const;
    QString getCacheImageFilePath() const;
    QString getLogFilePath() const;
    
private:
    Config();
    QJsonObject m_config;
};

} // namespace DiscordDrawRPC
