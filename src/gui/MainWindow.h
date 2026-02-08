#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QTimer>
#include <QImage>

namespace DiscordDrawRPC {

class CropWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    
protected:
    void closeEvent(QCloseEvent* event) override;
    
private slots:
    void takeScreenshot();
    void loadImage();
    void uploadToImgur();
    void updateDiscordStatus();
    void clearDiscordStatus();
    void setStartTimeToNow();
    void onUrlChanged(const QString& text);
    void updateDaemonStatus();
    void startDaemon();
    void stopDaemon();
    void showSettings();
    void viewLogs();
    void showAbout();
    
private:
    void initUi();
    void initTrayIcon();
    void launchTrayProcess();
    void storeGuiPid();
    void loadCurrentState();
    void updatePreview();
    QImage pixmapToImage(const QPixmap& pixmap);
    bool saveToCache(const QString& url, const QVector<qreal>& cropRectRatio);
    bool loadFromCache(const QString& url);
    
    // Wayland-specific
    bool detectWayland();
    void takeScreenshotWayland();
    bool checkCommand(const QString& cmd);
    void loadScreenshotFile(const QString& filepath);
    
    // X11/Windows screenshot
    void captureScreenshot();
    void checkSelection();
    
    // UI Components
    CropWidget* m_cropWidget;
    QPushButton* m_screenshotBtn;
    QPushButton* m_loadBtn;
    QPushButton* m_uploadBtn;
    QPushButton* m_updateBtn;
    QPushButton* m_clearBtn;
    QPushButton* m_nowBtn;
    QPushButton* m_startDaemonBtn;
    QPushButton* m_stopDaemonBtn;
    
    QLineEdit* m_urlInput;
    QLineEdit* m_detailInput;
    QLineEdit* m_stateInput;
    QDateTimeEdit* m_startTimeInput;
    
    QLabel* m_statusLabel;
    QLabel* m_daemonStatusLabel;
    
    QTimer* m_daemonCheckTimer;
    
    // Data
    QImage m_screenshot;
    QPixmap m_screenshotPixmap;
    QString m_uploadedUrl;
    bool m_isWayland;
    
    class ScreenshotSelector* m_selector;
};

} // namespace DiscordDrawRPC
