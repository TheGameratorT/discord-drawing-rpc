#include "MainWindow.h"
#include "SettingsDialog.h"
#include "CropWidget.h"
#include "ScreenshotSelector.h"
#include "LogViewerDialog.h"
#include "../common/Config.h"
#include "../common/Common.h"
#include "../common/PlatformUtils.h"
#include "../common/DaemonIPC.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMenuBar>
#include <QScreen>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QBuffer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QProcess>
#include <QDateTime>
#include <QApplication>
#include <QCloseEvent>
#include <QThread>

namespace DiscordDrawRPC {

// UI Constants
const QString DISCORD_BLUE = "#5865F2";
const QString DISCORD_BLUE_HOVER = "#4752C4";
const QString DISCORD_RED = "#ED4245";
const QString DISCORD_RED_HOVER = "#C03537";
const QString DISCORD_GREEN = "#43B581";
const QString DARK_BG = "#2b2b2b";
const QString DARK_GRAY = "#4E5058";

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_selector(nullptr)
    , m_daemonCheckTimer(nullptr)
{
    m_isWayland = detectWayland();
    storeGuiPid();
    
    initUi();
    
    // Start daemon status check timer
    m_daemonCheckTimer = new QTimer(this);
    connect(m_daemonCheckTimer, &QTimer::timeout, this, &MainWindow::updateDaemonStatus);
    m_daemonCheckTimer->start(1000);
    updateDaemonStatus();
    
    // Load current state
    loadCurrentState();
    
    // Initialize tray icon
    initTrayIcon();
    
    // Auto-start presence if enabled
    Config& config = Config::instance();
    config.load();
    bool autoStartPresence = config.getConfig().value("auto_start_presence").toBool();
    QString clientId = config.getValue("discord_client_id");
    
    // Don't auto-start if tray is running (means GUI was already running)
    if (autoStartPresence && !clientId.isEmpty() && !ProcessUtils::isDaemonRunning() && !ProcessUtils::isTrayRunning()) {
        QTimer::singleShot(500, this, &MainWindow::startDaemon);
    }
}

MainWindow::~MainWindow() {
    if (m_daemonCheckTimer) {
        m_daemonCheckTimer->stop();
    }
}

bool MainWindow::detectWayland() {
    QString waylandDisplay = qEnvironmentVariable("WAYLAND_DISPLAY");
    QString sessionType = qEnvironmentVariable("XDG_SESSION_TYPE");
    return !waylandDisplay.isEmpty() || sessionType == "wayland";
}

void MainWindow::storeGuiPid() {
    QString pidFile = Config::instance().getGuiPidFilePath();
    ProcessUtils::writePidFile(pidFile, QCoreApplication::applicationPid());
}

void MainWindow::initTrayIcon() {
    Config& config = Config::instance();
    config.load();
    
    bool enableTray = config.getConfig().value("enable_tray_icon").toBool();
    if (!enableTray) {
        // Stop tray if running
        if (ProcessUtils::isTrayRunning()) {
            ProcessUtils::terminateProcessFromPidFile(config.getTrayPidFilePath());
        }
        return;
    }
    
    // Launch tray if not already running
    if (!ProcessUtils::isTrayRunning()) {
        launchTrayProcess();
    }
}

void MainWindow::launchTrayProcess() {
    if (ProcessUtils::isTrayRunning()) {
        return;
    }
    
    QString trayPath = getExecutablePath(ExecutableType::Tray);
    
    QProcess* process = new QProcess(this);
    
#ifdef _WIN32
    process->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags |= CREATE_NO_WINDOW;
    });
#endif
    
    process->startDetached(trayPath);
}

void MainWindow::loadCurrentState() {
    QJsonObject stateData = DaemonIPC::readCurrentState();
    
    // Don't load if state is empty
    if (stateData.isEmpty()) {
        return;
    }
    
    // Populate URL field
    QString largeImage = stateData.value("large_image").toString();
    if (!largeImage.isEmpty()) {
        m_urlInput->setText(largeImage);
        m_uploadedUrl = largeImage;
        
        // Try to load the image from cache
        if (loadFromCache(largeImage)) {
            m_uploadBtn->setEnabled(true);
        }
    }
    
    // Populate details and state
    QString details = stateData.value("details").toString();
    if (!details.isEmpty() && details != "Sharing a screenshot") {
        m_detailInput->setText(details);
    }
    
    QString state = stateData.value("state").toString();
    if (!state.isEmpty()) {
        m_stateInput->setText(state);
    }
    
    // Populate start time
    qint64 startTimestamp = stateData.value("start").toVariant().toLongLong();
    if (startTimestamp > 0) {
        QDateTime startDateTime = QDateTime::fromSecsSinceEpoch(startTimestamp);
        m_startTimeInput->setDateTime(startDateTime);
    }
    
    // Update status label
    if (!largeImage.isEmpty()) {
        m_statusLabel->setText("✅ Loaded current Discord state");
    }
}

bool MainWindow::saveToCache(const QString& url, const QVector<qreal>& cropRectRatio) {
    if (m_screenshot.isNull()) {
        return false;
    }
    
    QString cacheImageFile = Config::instance().getCacheImageFilePath();
    QString cacheFile = Config::instance().getCacheFilePath();
    
    // Save image
    if (!m_screenshot.save(cacheImageFile, "PNG")) {
        return false;
    }
    
    // Save metadata
    QJsonObject cacheData;
    cacheData["url"] = url;
    
    QJsonArray ratioArray;
    for (qreal val : cropRectRatio) {
        ratioArray.append(val);
    }
    cacheData["crop_rect_ratio"] = ratioArray;
    
    QFile file(cacheFile);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QJsonDocument doc(cacheData);
    file.write(doc.toJson());
    file.close();
    
    return true;
}

bool MainWindow::loadFromCache(const QString& url) {
    QString cacheImageFile = Config::instance().getCacheImageFilePath();
    QString cacheFile = Config::instance().getCacheFilePath();
    
    QFile metaFile(cacheFile);
    if (!metaFile.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray data = metaFile.readAll();
    metaFile.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }
    
    QJsonObject cacheData = doc.object();
    
    // Check if URL matches
    if (cacheData.value("url").toString() != url) {
        return false;
    }
    
    // Load image
    if (!m_screenshot.load(cacheImageFile)) {
        return false;
    }
    
    m_screenshotPixmap = QPixmap::fromImage(m_screenshot);
    updatePreview();
    
    // Restore crop coordinates if available
    QJsonArray ratioArray = cacheData.value("crop_rect_ratio").toArray();
    if (ratioArray.size() == 3) {
        m_cropWidget->cropRectRatio.clear();
        for (const QJsonValue& val : ratioArray) {
            m_cropWidget->cropRectRatio.append(val.toDouble());
        }
        
        // Recalculate crop_rect from ratio
        QRect imgBounds = m_cropWidget->getImageDisplayBounds();
        if (imgBounds.width() > 0 && imgBounds.height() > 0) {
            int cropX = imgBounds.x() + static_cast<int>(m_cropWidget->cropRectRatio[0] * imgBounds.width());
            int cropY = imgBounds.y() + static_cast<int>(m_cropWidget->cropRectRatio[1] * imgBounds.height());
            int squareSize = static_cast<int>(m_cropWidget->cropRectRatio[2] * imgBounds.width());
            m_cropWidget->cropRect = QRect(cropX, cropY, squareSize, squareSize);
            m_cropWidget->update();
        }
    }
    
    return true;
}

void MainWindow::initUi() {
    setWindowTitle("Discord Drawing RPC");
    setWindowIcon(QIcon(":/icons/icon.png"));
    setGeometry(100, 100, 1000, 700);
    
    // Add menu bar
    QMenuBar* menuBar = new QMenuBar(this);
    QMenu* fileMenu = menuBar->addMenu("&File");
    QAction* settingsAction = fileMenu->addAction("&Settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
    
    fileMenu->addSeparator();
    QAction* quitAction = fileMenu->addAction("&Quit");
    connect(quitAction, &QAction::triggered, this, &MainWindow::quitApplication);
    
    QMenu* aboutMenu = menuBar->addMenu("&About");
    QAction* aboutQtAction = aboutMenu->addAction("&Qt");
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
    QAction* aboutThisAction = aboutMenu->addAction("&This");
    connect(aboutThisAction, &QAction::triggered, this, &MainWindow::showAbout);
    
    setMenuBar(menuBar);
    
    // Central widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Main layout - horizontal split
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    
    // Left side - Preview area
    QVBoxLayout* previewContainer = new QVBoxLayout();
    
    QLabel* previewLabel = new QLabel("Screenshot Preview", this);
    previewLabel->setStyleSheet(QString("font-weight: bold; font-size: 18px; padding: 10px; color: %1;").arg(DISCORD_BLUE));
    previewLabel->setAlignment(Qt::AlignCenter);
    previewContainer->addWidget(previewLabel);
    
    m_cropWidget = new CropWidget(this);
    previewContainer->addWidget(m_cropWidget, 1);
    
    // Image Source Group
    QGroupBox* sourceGroup = new QGroupBox("Image Source", this);
    QVBoxLayout* sourceLayout = new QVBoxLayout(sourceGroup);
    
    m_screenshotBtn = new QPushButton("📷 Take Screenshot", this);
    connect(m_screenshotBtn, &QPushButton::clicked, this, &MainWindow::takeScreenshot);
    m_screenshotBtn->setMinimumHeight(35);
    m_screenshotBtn->setStyleSheet(QString(
        "QPushButton {"
        "    background-color: %1;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 8px;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %2;"
        "}"
    ).arg(DISCORD_BLUE, DISCORD_BLUE_HOVER));
    sourceLayout->addWidget(m_screenshotBtn);
    
    m_loadBtn = new QPushButton("📁 Load Image File", this);
    connect(m_loadBtn, &QPushButton::clicked, this, &MainWindow::loadImage);
    m_loadBtn->setMinimumHeight(35);
    m_loadBtn->setStyleSheet(QString(
        "QPushButton {"
        "    background-color: %1;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 8px;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %2;"
        "}"
    ).arg(DISCORD_BLUE, DISCORD_BLUE_HOVER));
    sourceLayout->addWidget(m_loadBtn);
    
    m_uploadBtn = new QPushButton("☁️ Upload to Imgur", this);
    connect(m_uploadBtn, &QPushButton::clicked, this, &MainWindow::uploadToImgur);
    m_uploadBtn->setEnabled(false);
    m_uploadBtn->setMinimumHeight(35);
    m_uploadBtn->setStyleSheet(QString(
        "QPushButton {"
        "    background-color: %1;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 8px;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3ba372;"
        "}"
        "QPushButton:disabled {"
        "    background-color: %2;"
        "}"
    ).arg(DISCORD_GREEN, DARK_GRAY));
    sourceLayout->addWidget(m_uploadBtn);
    
    previewContainer->addWidget(sourceGroup);
    
    mainLayout->addLayout(previewContainer, 3);
    
    // Right side - Controls panel
    QVBoxLayout* controlsContainer = new QVBoxLayout();
    controlsContainer->setSpacing(10);
    
    // Title
    QLabel* title = new QLabel("Discord Drawing RPC", this);
    title->setStyleSheet(QString("font-size: 18px; font-weight: bold; padding: 10px; color: %1;").arg(DISCORD_BLUE));
    title->setAlignment(Qt::AlignCenter);
    controlsContainer->addWidget(title);
    
    // Daemon Control Group
    QGroupBox* daemonGroup = new QGroupBox("Presence Control", this);
    QVBoxLayout* daemonLayout = new QVBoxLayout(daemonGroup);
    
    m_daemonStatusLabel = new QLabel("Status: Checking...", this);
    m_daemonStatusLabel->setStyleSheet("padding: 5px; font-weight: bold;");
    daemonLayout->addWidget(m_daemonStatusLabel);
    
    QHBoxLayout* daemonBtnLayout = new QHBoxLayout();
    
    m_startDaemonBtn = new QPushButton("Start Presence", this);
    connect(m_startDaemonBtn, &QPushButton::clicked, this, &MainWindow::startDaemon);
    m_startDaemonBtn->setMinimumHeight(35);
    m_startDaemonBtn->setStyleSheet(QString(
        "QPushButton {"
        "    background-color: %1;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 8px;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3ba372;"
        "}"
    ).arg(DISCORD_GREEN));
    daemonBtnLayout->addWidget(m_startDaemonBtn);
    
    m_stopDaemonBtn = new QPushButton("Stop Presence", this);
    connect(m_stopDaemonBtn, &QPushButton::clicked, this, &MainWindow::stopDaemon);
    m_stopDaemonBtn->setMinimumHeight(35);
    m_stopDaemonBtn->setStyleSheet(QString(
        "QPushButton {"
        "    background-color: %1;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 8px;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %2;"
        "}"
    ).arg(DISCORD_RED, DISCORD_RED_HOVER));
    daemonBtnLayout->addWidget(m_stopDaemonBtn);
    
    daemonLayout->addLayout(daemonBtnLayout);
    
    // View Logs button
    QPushButton* viewLogsBtn = new QPushButton("📋 View Logs", this);
    connect(viewLogsBtn, &QPushButton::clicked, this, &MainWindow::viewLogs);
    viewLogsBtn->setMinimumHeight(35);
    viewLogsBtn->setStyleSheet(QString(
        "QPushButton {"
        "    background-color: %1;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 8px;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3a3d42;"
        "}"
    ).arg(DARK_GRAY));
    daemonLayout->addWidget(viewLogsBtn);
    
    controlsContainer->addWidget(daemonGroup);
    
    // Discord Status Group
    QGroupBox* statusGroup = new QGroupBox("Discord Status", this);
    QVBoxLayout* statusLayout = new QVBoxLayout(statusGroup);
    
    QLabel* urlLabel = new QLabel("Image URL:", this);
    urlLabel->setStyleSheet("font-weight: bold;");
    statusLayout->addWidget(urlLabel);
    
    m_urlInput = new QLineEdit(this);
    m_urlInput->setPlaceholderText("Upload or paste URL here");
    connect(m_urlInput, &QLineEdit::textChanged, this, &MainWindow::onUrlChanged);
    statusLayout->addWidget(m_urlInput);
    
    statusLayout->addSpacing(10);
    
    QLabel* detailLabel = new QLabel("Details:", this);
    detailLabel->setStyleSheet("font-weight: bold;");
    statusLayout->addWidget(detailLabel);
    
    m_detailInput = new QLineEdit(this);
    m_detailInput->setPlaceholderText("What are you doing?");
    statusLayout->addWidget(m_detailInput);
    
    QLabel* stateLabel = new QLabel("State:", this);
    stateLabel->setStyleSheet("font-weight: bold;");
    statusLayout->addWidget(stateLabel);
    
    m_stateInput = new QLineEdit(this);
    m_stateInput->setPlaceholderText("Additional status info");
    statusLayout->addWidget(m_stateInput);
    
    statusLayout->addSpacing(10);
    
    QLabel* startTimeLabel = new QLabel("Start Time:", this);
    startTimeLabel->setStyleSheet("font-weight: bold;");
    statusLayout->addWidget(startTimeLabel);
    
    QHBoxLayout* timeLayout = new QHBoxLayout();
    
    m_startTimeInput = new QDateTimeEdit(this);
    m_startTimeInput->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    m_startTimeInput->setDateTime(QDateTime::currentDateTime());
    m_startTimeInput->setCalendarPopup(true);
    timeLayout->addWidget(m_startTimeInput, 1);
    
    m_nowBtn = new QPushButton("Now", this);
    connect(m_nowBtn, &QPushButton::clicked, this, &MainWindow::setStartTimeToNow);
    m_nowBtn->setMinimumHeight(30);
    m_nowBtn->setStyleSheet(QString(
        "QPushButton {"
        "    background-color: %1;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 5px;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %2;"
        "}"
    ).arg(DISCORD_BLUE, DISCORD_BLUE_HOVER));
    timeLayout->addWidget(m_nowBtn);
    
    statusLayout->addLayout(timeLayout);
    
    controlsContainer->addWidget(statusGroup);
    
    // Action buttons
    controlsContainer->addSpacing(10);
    
    m_updateBtn = new QPushButton("Set Status", this);
    connect(m_updateBtn, &QPushButton::clicked, this, &MainWindow::updateDiscordStatus);
    m_updateBtn->setEnabled(false);
    m_updateBtn->setMinimumHeight(40);
    m_updateBtn->setStyleSheet(QString(
        "QPushButton {"
        "    background-color: %1;"
        "    color: white;"
        "    font-weight: bold;"
        "    padding: 10px;"
        "    border-radius: 5px;"
        "    font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %2;"
        "}"
        "QPushButton:disabled {"
        "    background-color: %3;"
        "}"
    ).arg(DISCORD_BLUE, DISCORD_BLUE_HOVER, DARK_GRAY));
    controlsContainer->addWidget(m_updateBtn);
    
    // Status label
    controlsContainer->addSpacing(10);
    m_statusLabel = new QLabel("Ready", this);
    m_statusLabel->setStyleSheet(QString(
        "padding: 10px;"
        "border-radius: 5px;"
        "background-color: %1;"
        "border: 1px solid %2;"
    ).arg(DARK_BG, DISCORD_BLUE));
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    controlsContainer->addWidget(m_statusLabel);
    
    controlsContainer->addStretch();
    
    mainLayout->addLayout(controlsContainer, 1);
}

void MainWindow::takeScreenshot() {
    if (m_isWayland) {
        takeScreenshotWayland();
    } else {
        m_statusLabel->setText("Select area to screenshot...");
        QTimer::singleShot(100, this, &MainWindow::captureScreenshot);
    }
}

void MainWindow::takeScreenshotWayland() {
    m_statusLabel->setText("Taking screenshot with external tool...");
    
    // Create temp file
    QString tempFile = QDir::tempPath() + "/discord_rpc_screenshot.png";
    
    // Check for grim + slurp
    if (checkCommand("grim") && checkCommand("slurp")) {
        QProcess slurp;
        slurp.start("slurp");
        slurp.waitForFinished();
        
        if (slurp.exitCode() == 0) {
            QString geometry = QString::fromUtf8(slurp.readAllStandardOutput()).trimmed();
            QProcess::execute("grim", QStringList() << "-g" << geometry << tempFile);
            loadScreenshotFile(tempFile);
            return;
        }
    }
    
    // Try gnome-screenshot
    if (checkCommand("gnome-screenshot")) {
        QProcess process;
        process.start("gnome-screenshot", QStringList() << "-a" << "-f" << tempFile);
        process.waitForFinished();
        
        if (process.exitCode() == 0) {
            loadScreenshotFile(tempFile);
            return;
        }
    }
    
    // Try spectacle
    if (checkCommand("spectacle")) {
        QProcess process;
        process.start("spectacle", QStringList() << "-r" << "-b" << "-n" << "-o" << tempFile);
        process.waitForFinished();
        
        if (process.exitCode() == 0) {
            loadScreenshotFile(tempFile);
            return;
        }
    }
    
    // No tool worked
    QMessageBox::critical(
        this,
        "Screenshot Failed",
        "Could not take screenshot. Please install one of:\n"
        "- gnome-screenshot\n"
        "- spectacle (KDE)\n"
        "- grim and slurp (wlroots/Sway)\n\n"
        "Or use 'Load Image' to select an existing file."
    );
    m_statusLabel->setText("Screenshot failed - please install screenshot tool");
}

bool MainWindow::checkCommand(const QString& cmd) {
    QProcess process;
    process.start("which", QStringList() << cmd);
    process.waitForFinished();
    return process.exitCode() == 0;
}

void MainWindow::loadScreenshotFile(const QString& filepath) {
    if (!QFile::exists(filepath)) {
        return;
    }
    
    m_screenshotPixmap.load(filepath);
    m_screenshot.load(filepath);
    updatePreview();
    m_uploadBtn->setEnabled(true);
    m_statusLabel->setText("Screenshot captured! Click 'Upload to Imgur' to upload.");
    
    // Clean up temp file
    QFile::remove(filepath);
}

void MainWindow::captureScreenshot() {
    QScreen* screen = QApplication::primaryScreen();
    if (!screen) return;
    
    QPixmap screenshot = screen->grabWindow(0);
    
    m_selector = new ScreenshotSelector(screenshot);
    m_selector->show();
    
    QTimer::singleShot(100, this, &MainWindow::checkSelection);
}

void MainWindow::checkSelection() {
    if (m_selector && !m_selector->isVisible()) {
        QRect rect = m_selector->getRect();
        
        if (rect.width() > 10 && rect.height() > 10) {
            QScreen* screen = QApplication::primaryScreen();
            if (!screen) return;
            
            QPixmap fullScreenshot = screen->grabWindow(0);
            m_screenshotPixmap = fullScreenshot.copy(rect);
            m_screenshot = m_screenshotPixmap.toImage();
            
            updatePreview();
            m_uploadBtn->setEnabled(true);
            m_statusLabel->setText("Screenshot captured! Click 'Upload to Imgur' to upload.");
        } else {
            m_statusLabel->setText("Screenshot cancelled or too small");
        }
        
        m_selector->deleteLater();
        m_selector = nullptr;
    } else {
        QTimer::singleShot(100, this, &MainWindow::checkSelection);
    }
}

void MainWindow::loadImage() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open Image",
        "",
        "Image Files (*.png *.jpg *.jpeg *.bmp *.gif)"
    );
    
    if (!fileName.isEmpty()) {
        m_screenshotPixmap.load(fileName);
        m_screenshot.load(fileName);
        updatePreview();
        m_uploadBtn->setEnabled(true);
        m_statusLabel->setText("Loaded " + QFileInfo(fileName).fileName());
    }
}

void MainWindow::updatePreview() {
    if (!m_screenshotPixmap.isNull()) {
        m_cropWidget->setImage(m_screenshotPixmap);
    }
}

void MainWindow::uploadToImgur() {
    if (m_screenshot.isNull()) {
        QMessageBox::warning(this, "No Screenshot", "Please take a screenshot first!");
        return;
    }
    
    m_statusLabel->setText("Uploading to Imgur...");
    m_uploadBtn->setEnabled(false);
    
    Config& config = Config::instance();
    QString imgurClientId = config.getValue("imgur_client_id");
    
    if (imgurClientId.isEmpty()) {
        QMessageBox::warning(this, "No Imgur Client ID", "Please configure Imgur Client ID in Settings!");
        m_uploadBtn->setEnabled(true);
        return;
    }
    
    // Get crop rectangle and crop the image
    QRect cropRect = m_cropWidget->getCropRectOnOriginal();
    QImage croppedImage;
    
    if (!cropRect.isNull()) {
        croppedImage = m_screenshot.copy(cropRect);
    } else {
        croppedImage = m_screenshot;
    }
    
    // Convert to PNG bytes
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    croppedImage.save(&buffer, "PNG");
    buffer.close();
    
    // Upload to Imgur
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    
    QNetworkRequest request(QUrl("https://api.imgur.com/3/image"));
    request.setRawHeader("Authorization", QString("Client-ID %1").arg(imgurClientId).toUtf8());
    
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                       QVariant("form-data; name=\"image\"; filename=\"screenshot.png\""));
    imagePart.setBody(imageData);
    
    multiPart->append(imagePart);
    
    QNetworkReply* reply = manager->post(request, multiPart);
    multiPart->setParent(reply);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        reply->deleteLater();
        manager->deleteLater();
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                QJsonObject data = obj.value("data").toObject();
                QString link = data.value("link").toString();
                
                if (!link.isEmpty()) {
                    m_uploadedUrl = link;
                    m_urlInput->setText(m_uploadedUrl);
                    m_statusLabel->setText("✅ Uploaded successfully!");
                    m_updateBtn->setEnabled(true);
                    
                    // Save to cache
                    saveToCache(m_uploadedUrl, m_cropWidget->cropRectRatio);
                } else {
                    m_statusLabel->setText("❌ Upload failed: Invalid response");
                    m_uploadBtn->setEnabled(true);
                }
            }
        } else {
            QString error = reply->errorString();
            QMessageBox::critical(this, "Upload Error", "Failed to upload screenshot:\n" + error);
            m_statusLabel->setText("❌ Upload failed: " + error);
            m_uploadBtn->setEnabled(true);
        }
    });
}

void MainWindow::onUrlChanged(const QString& text) {
    m_updateBtn->setEnabled(!text.trimmed().isEmpty());
}

void MainWindow::setStartTimeToNow() {
    m_startTimeInput->setDateTime(QDateTime::currentDateTime());
}

void MainWindow::updateDaemonStatus() {
    if (ProcessUtils::isDaemonRunning()) {
        m_daemonStatusLabel->setText("Status: Running ✅");
        m_daemonStatusLabel->setStyleSheet(QString("padding: 5px; font-weight: bold; color: %1;").arg(DISCORD_GREEN));
        m_startDaemonBtn->setEnabled(false);
        m_stopDaemonBtn->setEnabled(true);
    } else {
        m_daemonStatusLabel->setText("Status: Stopped ⏸");
        m_daemonStatusLabel->setStyleSheet("padding: 5px; font-weight: bold; color: #999;");
        m_startDaemonBtn->setEnabled(true);
        m_stopDaemonBtn->setEnabled(false);
    }
}

void MainWindow::startDaemon() {
    if (ProcessUtils::isDaemonRunning()) {
        QMessageBox::information(this, "Already Running", "Presence is already running!");
        return;
    }
    
    // Check if client ID is set
    Config& config = Config::instance();
    config.load();
    QString clientId = config.getValue("discord_client_id");
    
    if (clientId.isEmpty()) {
        QMessageBox::warning(
            this,
            "No Client ID",
            "Discord Client ID is not configured!\n\nPlease go to Settings and set your Discord Client ID first."
        );
        return;
    }
    
    QString daemonPath = getExecutablePath(ExecutableType::Daemon);
    
    if (!QFile::exists(daemonPath)) {
        QMessageBox::critical(
            this,
            "Presence Not Found",
            QString("Could not find presence service at:\n%1\n\nMake sure the presence executable is in the same directory.").arg(daemonPath)
        );
        return;
    }
    
    // Ensure the state file has command="update" before starting daemon
    // This prevents the daemon from immediately quitting if command was "quit"
    DaemonIPC::setUpdateCommand();
    
    QProcess* process = new QProcess(this);
    
#ifdef _WIN32
    process->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags |= CREATE_NO_WINDOW;
    });
#endif
    
    process->startDetached(daemonPath);
    
    // Wait a moment and check if it started
    QTimer::singleShot(500, this, [this]() {
        updateDaemonStatus();
        // if (ProcessUtils::isDaemonRunning()) {
        //     QMessageBox::information(this, "Success", "Presence started successfully!");
        // } else {
        //     QMessageBox::warning(this, "Warning", "Presence may have failed to start. Check if Discord is running.");
        // }
    });
}

void MainWindow::stopDaemon() {
    if (!ProcessUtils::isDaemonRunning()) {
        QMessageBox::information(this, "Not Running", "Presence is not running!");
        return;
    }
    
    DaemonIPC::sendQuitCommand();
    
    // Wait and check
    QTimer::singleShot(1000, this, [this]() {
        updateDaemonStatus();
        // if (!ProcessUtils::isDaemonRunning()) {
        //     QMessageBox::information(this, "Success", "Presence stopped!");
        // } else {
        //     QMessageBox::warning(this, "Warning", "Presence may still be running.");
        // }
    });
}

void MainWindow::updateDiscordStatus() {
    QString url = m_urlInput->text().trimmed();
    QString details = m_detailInput->text().trimmed();
    QString state = m_stateInput->text().trimmed();
    
    if (url.isEmpty()) {
        QMessageBox::warning(this, "No URL", "Please provide an image URL!");
        return;
    }
    
    qint64 startTime = m_startTimeInput->dateTime().toSecsSinceEpoch();
    
    // Check if daemon is running
    if (!ProcessUtils::isDaemonRunning()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Daemon Not Running",
            "The daemon is not running. Would you like to start it?",
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            startDaemon();
            // Wait for daemon to start (poll up to 2 seconds)
            for (int i = 0; i < 20 && !ProcessUtils::isDaemonRunning(); ++i) {
                QThread::msleep(100);
            }
        }
    }
    
    // Send update command
    bool success = DaemonIPC::sendUpdateCommand(
        url,
        "Screenshot",
        details.isEmpty() ? "Sharing a screenshot" : details,
        state,
        startTime
    );
    
    if (success) {
        m_statusLabel->setText("✅ Discord status updated!");
        QMessageBox::information(this, "Success", "Discord status updated!");
    } else {
        QMessageBox::critical(this, "Error", "Failed to update status!");
        m_statusLabel->setText("❌ Error: Failed to write state file");
    }
}

void MainWindow::quitApplication() {
    // Stop daemon if running
    if (ProcessUtils::isDaemonRunning()) {
        stopDaemon();
    }
    
    // Stop tray if running
    if (ProcessUtils::isTrayRunning()) {
        ProcessUtils::terminateProcessFromPidFile(Config::instance().getTrayPidFilePath());
    }
    
    // Clean up PID file
    QString guiPidFile = Config::instance().getGuiPidFilePath();
    QFile::remove(guiPidFile);
    
    QApplication::quit();
}

void MainWindow::showSettings() {
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QJsonObject settings = dialog.getSettings();
        
        // Validate settings
        if (settings.value("discord_client_id").toString().isEmpty() || 
            settings.value("imgur_client_id").toString().isEmpty()) {
            QMessageBox::warning(
                this,
                "Invalid Settings",
                "Both Discord and Imgur Client IDs are required!"
            );
            return;
        }
        
        // Save settings
        Config& config = Config::instance();
        
        // Check if Discord Client ID changed
        QString oldClientId = config.getValue("discord_client_id");
        QString newClientId = settings.value("discord_client_id").toString();
        bool clientIdChanged = (oldClientId != newClientId);
        
        config.setConfig(settings);
        
        if (config.save()) {
            // Re-initialize tray icon based on new setting
            initTrayIcon();
            
            QString message = "Settings saved successfully!";
            if (clientIdChanged) {
                message += "\n\nPlease restart the presence for changes to take effect.";
            }
            
            QMessageBox::information(
                this,
                "Settings Saved",
                message
            );
        } else {
            QMessageBox::critical(
                this,
                "Error",
                "Failed to save settings!"
            );
        }
    }
}

void MainWindow::viewLogs() {
    LogViewerDialog* logDialog = new LogViewerDialog(this);
    logDialog->setAttribute(Qt::WA_DeleteOnClose);
    logDialog->show();
}

void MainWindow::showAbout() {
    QString bodyText = "<p><strong>Discord Drawing RPC</strong></p>"
        "<p>Copyright &copy; 2026 TheGameratorT</p>"
        "<p><span style=\"text-decoration: underline;\">License:</span></p>"
        "<p style=\"padding-left: 30px;\">This application is licensed under the GNU General Public License v3.</p>"
        "<p style=\"padding-left: 30px;\">This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.</p>"
        "<p style=\"padding-left: 30px;\">This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.<br />See the GNU General Public License for more details.</p>"
        "<p style=\"padding-left: 30px;\">For details read the LICENSE file bundled with the program or visit:</p>"
        "<p style=\"padding-left: 30px;\"><a href=\"https://www.gnu.org/licenses/\">https://www.gnu.org/licenses/</a></p>";
    
    QMessageBox::about(this, "About Discord Drawing RPC", bodyText);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_daemonCheckTimer) {
        m_daemonCheckTimer->stop();
    }
    
    // Check config option for stopping daemon on close
    Config& config = Config::instance();
    config.load();
    bool stopDaemonOnClose = config.getConfig().value("stop_daemon_on_close").toBool();
    
    if (stopDaemonOnClose && ProcessUtils::isDaemonRunning()) {
        stopDaemon();
    }
    
    // Clean up PID file
    QString guiPidFile = Config::instance().getGuiPidFilePath();
    QFile::remove(guiPidFile);
    
    event->accept();
}

} // namespace DiscordDrawRPC
