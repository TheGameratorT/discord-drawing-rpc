#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QJsonObject>

namespace DiscordDrawRPC {

class SettingsDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    
    QJsonObject getSettings() const;
    
private:
    void loadCurrentSettings();
    
    QLineEdit* m_discordIdInput;
    QLineEdit* m_imgurIdInput;
    QCheckBox* m_enableTrayCheckbox;
    QCheckBox* m_stopDaemonOnCloseCheckbox;
    QCheckBox* m_autoStartPresenceCheckbox;
};

} // namespace DiscordDrawRPC
