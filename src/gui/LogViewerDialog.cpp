#include "LogViewerDialog.h"
#include "../common/Config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QScrollBar>
#include <QFileInfo>

namespace DiscordDrawRPC {

LogViewerDialog::LogViewerDialog(QWidget* parent)
    : QDialog(parent)
    , m_lastFileSize(0)
{
    setWindowTitle("Presence Logs");
    setMinimumSize(800, 600);
    
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Log view
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setStyleSheet(
        "QTextEdit {"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 10pt;"
        "    background-color: #1e1e1e;"
        "    color: #d4d4d4;"
        "    border: 1px solid #444;"
        "}"
    );
    mainLayout->addWidget(m_logView);
    
    // Button layout
    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    m_refreshBtn = new QPushButton("🔄 Refresh", this);
    connect(m_refreshBtn, &QPushButton::clicked, this, &LogViewerDialog::refreshLogs);
    m_refreshBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #5865F2;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 8px 15px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #4752C4;"
        "}"
    );
    btnLayout->addWidget(m_refreshBtn);
    
    m_clearBtn = new QPushButton("🗑️ Clear Logs", this);
    connect(m_clearBtn, &QPushButton::clicked, this, &LogViewerDialog::clearLogs);
    m_clearBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #ED4245;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 8px 15px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #C03537;"
        "}"
    );
    btnLayout->addWidget(m_clearBtn);
    
    btnLayout->addStretch();
    
    m_closeBtn = new QPushButton("Close", this);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::close);
    m_closeBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #4E5058;"
        "    color: white;"
        "    padding: 8px 15px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3a3d42;"
        "}"
    );
    btnLayout->addWidget(m_closeBtn);
    
    mainLayout->addLayout(btnLayout);
    
    // Setup auto-refresh timer
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &LogViewerDialog::loadLogs);
    m_refreshTimer->start(1000); // Update every second
    
    // Setup file watcher
    m_fileWatcher = new QFileSystemWatcher(this);
    QString logPath = Config::instance().getLogFilePath();
    if (QFile::exists(logPath)) {
        m_fileWatcher->addPath(logPath);
    }
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &LogViewerDialog::onLogFileChanged);
    
    // Load initial logs
    loadLogs();
}

LogViewerDialog::~LogViewerDialog() {
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
}

void LogViewerDialog::loadLogs() {
    QString logPath = Config::instance().getLogFilePath();
    
    QFile logFile(logPath);
    if (!logFile.exists()) {
        m_logView->setPlainText("No log file found. Start the presence to generate logs.");
        return;
    }
    
    QFileInfo fileInfo(logFile);
    qint64 currentSize = fileInfo.size();
    
    // Only update if file size changed
    if (currentSize == m_lastFileSize) {
        return;
    }
    
    m_lastFileSize = currentSize;
    
    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_logView->setPlainText("Error: Cannot open log file for reading.");
        return;
    }
    
    // Read the last 10000 lines to avoid memory issues
    QTextStream in(&logFile);
    QStringList lines;
    
    while (!in.atEnd()) {
        QString line = in.readLine();
        lines.append(line);
        
        // Keep only last 10000 lines
        if (lines.size() > 10000) {
            lines.removeFirst();
        }
    }
    
    logFile.close();
    
    // Save scroll position
    QScrollBar* scrollBar = m_logView->verticalScrollBar();
    bool wasAtBottom = scrollBar->value() == scrollBar->maximum();
    
    // Update text
    m_logView->setPlainText(lines.join("\n"));
    
    // Auto-scroll to bottom if we were already at bottom
    if (wasAtBottom) {
        QTimer::singleShot(0, [this]() {
            QScrollBar* scrollBar = m_logView->verticalScrollBar();
            scrollBar->setValue(scrollBar->maximum());
        });
    }
}

void LogViewerDialog::refreshLogs() {
    m_lastFileSize = 0; // Force reload
    loadLogs();
}

void LogViewerDialog::clearLogs() {
    QString logPath = Config::instance().getLogFilePath();
    
    QFile logFile(logPath);
    if (logFile.exists()) {
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            logFile.close();
            m_lastFileSize = 0;
            m_logView->setPlainText("Logs cleared.");
        } else {
            m_logView->append("\nError: Cannot clear log file.");
        }
    }
}

void LogViewerDialog::onLogFileChanged() {
    // Re-add the path if it was removed (happens on some systems)
    QString logPath = Config::instance().getLogFilePath();
    if (!m_fileWatcher->files().contains(logPath) && QFile::exists(logPath)) {
        m_fileWatcher->addPath(logPath);
    }
    
    loadLogs();
}

} // namespace DiscordDrawRPC
