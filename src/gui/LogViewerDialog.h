#pragma once

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QFileSystemWatcher>

namespace DiscordDrawRPC {

class LogViewerDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit LogViewerDialog(QWidget* parent = nullptr);
    ~LogViewerDialog();
    
private slots:
    void refreshLogs();
    void clearLogs();
    void onLogFileChanged();
    
private:
    void loadLogs();
    
    QTextEdit* m_logView;
    QPushButton* m_refreshBtn;
    QPushButton* m_clearBtn;
    QPushButton* m_closeBtn;
    QTimer* m_refreshTimer;
    QFileSystemWatcher* m_fileWatcher;
    qint64 m_lastFileSize;
};

} // namespace DiscordDrawRPC
