#include "DaemonIPC.h"
#include "Config.h"

#include <QFile>
#include <QJsonDocument>
#include <QDebug>

namespace DiscordDrawRPC {

bool DaemonIPC::sendQuitCommand() {
    // Read existing state to preserve fields
    QJsonObject stateData = readStateFile();
    
    // Set quit command
    stateData["command"] = "quit";
    
    return writeStateFile(stateData);
}

bool DaemonIPC::sendUpdateCommand(
    const QString& largeImage,
    const QString& largeText,
    const QString& details,
    const QString& state,
    qint64 startTimestamp)
{
    QJsonObject stateData;
    stateData["command"] = "update";
    stateData["large_image"] = largeImage;
    stateData["large_text"] = largeText;
    stateData["details"] = details;
    stateData["state"] = state;
    stateData["start"] = startTimestamp;
    
    return writeStateFile(stateData);
}

QJsonObject DaemonIPC::readCurrentState() {
    return readStateFile();
}

QJsonObject DaemonIPC::readStateFile() {
    QString stateFile = Config::instance().getStateFilePath();
    
    QFile file(stateFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return QJsonObject();
    }
    
    return doc.object();
}

bool DaemonIPC::writeStateFile(const QJsonObject& data) {
    QString stateFile = Config::instance().getStateFilePath();
    
    QFile file(stateFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open state file for writing:" << stateFile;
        return false;
    }
    
    QJsonDocument doc(data);
    qint64 bytesWritten = file.write(doc.toJson());
    file.close();
    
    return bytesWritten > 0;
}

} // namespace DiscordDrawRPC
