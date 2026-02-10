#include "SettingsDialog.h"
#include "../common/Config.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>

namespace DiscordDrawRPC {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setMinimumWidth(500);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // Form layout for settings
    QFormLayout* formLayout = new QFormLayout();
    
    // Discord Client ID
    m_discordIdInput = new QLineEdit(this);
    m_discordIdInput->setPlaceholderText("Enter Discord Client ID");
    formLayout->addRow("Discord Client ID:", m_discordIdInput);
    
    // Imgur Client ID
    m_imgurIdInput = new QLineEdit(this);
    m_imgurIdInput->setPlaceholderText("Enter Imgur Client ID");
    formLayout->addRow("Imgur Client ID:", m_imgurIdInput);
    
    // Enable Tray Icon
    m_enableTrayCheckbox = new QCheckBox("Show system tray icon", this);
    m_enableTrayCheckbox->setToolTip("Enable to minimize to system tray instead of closing");
    formLayout->addRow("Tray Icon:", m_enableTrayCheckbox);
    
    // Stop Daemon on Close
    m_stopDaemonOnCloseCheckbox = new QCheckBox("Stop daemon when closing GUI", this);
    m_stopDaemonOnCloseCheckbox->setToolTip("Automatically stop the Discord presence daemon when closing the GUI window");
    formLayout->addRow("Stop Daemon:", m_stopDaemonOnCloseCheckbox);
    
    layout->addLayout(formLayout);
    
    // Help text
    QLabel* helpText = new QLabel(
        "<b>How to get Client IDs:</b><br><br>"
        "<b>Discord:</b> Visit <a href='https://discord.com/developers/applications'>Discord Developer Portal</a>, "
        "create an application, and copy the Application ID.<br><br>"
        "<b>Imgur:</b> Visit <a href='https://api.imgur.com/oauth2/addclient'>Imgur API</a>, "
        "register an application (choose 'OAuth 2 authorization without a callback URL'), and copy the Client ID.",
        this
    );
    helpText->setWordWrap(true);
    helpText->setOpenExternalLinks(true);
    helpText->setStyleSheet("padding: 10px; border-radius: 5px;");
    layout->addWidget(helpText);
    
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    // Load current values
    loadCurrentSettings();
}

void SettingsDialog::loadCurrentSettings() {
    Config& config = Config::instance();
    m_discordIdInput->setText(config.getValue("discord_client_id"));
    m_imgurIdInput->setText(config.getValue("imgur_client_id"));
    m_enableTrayCheckbox->setChecked(config.getConfig().value("enable_tray_icon").toBool());
    m_stopDaemonOnCloseCheckbox->setChecked(config.getConfig().value("stop_daemon_on_close").toBool());
}

QJsonObject SettingsDialog::getSettings() const {
    QJsonObject settings;
    settings["discord_client_id"] = m_discordIdInput->text().trimmed();
    settings["imgur_client_id"] = m_imgurIdInput->text().trimmed();
    settings["enable_tray_icon"] = m_enableTrayCheckbox->isChecked();
    settings["stop_daemon_on_close"] = m_stopDaemonOnCloseCheckbox->isChecked();
    return settings;
}

} // namespace DiscordDrawRPC
