#include "Config.h"
#include "Common.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace DiscordDrawRPC {

Config::Config() {
    // Initialize with default values
    m_config["discord_client_id"] = "";
    m_config["imgur_client_id"] = "";
    m_config["enable_tray_icon"] = true;
}

Config& Config::instance() {
    static Config instance;
    return instance;
}

QString Config::getConfigFilePath() const {
    return getPlatformDirs().configDir + "/config.json";
}

QString Config::getStateFilePath() const {
    return getPlatformDirs().dataDir + "/state.json";
}

QString Config::getDaemonPidFilePath() const {
    return getPlatformDirs().dataDir + "/daemon.pid";
}

QString Config::getGuiPidFilePath() const {
    return getPlatformDirs().dataDir + "/gui.pid";
}

QString Config::getTrayPidFilePath() const {
    return getPlatformDirs().dataDir + "/tray.pid";
}

QString Config::getCacheFilePath() const {
    return getPlatformDirs().dataDir + "/image_cache.json";
}

QString Config::getCacheImageFilePath() const {
    return getPlatformDirs().dataDir + "/cached_image.png";
}

QString Config::getLogFilePath() const {
    return getPlatformDirs().dataDir + "/daemon.log";
}

bool Config::load() {
    QString configPath = getConfigFilePath();
    
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        // Config file doesn't exist, create it with defaults
        qDebug() << "Config file not found, creating with defaults";
        return save();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid config file format";
        return false;
    }
    
    m_config = doc.object();
    return true;
}

bool Config::save() {
    QString configPath = getConfigFilePath();
    
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open config file for writing:" << configPath;
        return false;
    }
    
    QJsonDocument doc(m_config);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    return true;
}

QString Config::getValue(const QString& key) const {
    return m_config.value(key).toString();
}

void Config::setValue(const QString& key, const QString& value) {
    m_config[key] = value;
}

void Config::setConfig(const QJsonObject& config) {
    m_config = config;
}

} // namespace DiscordDrawRPC
