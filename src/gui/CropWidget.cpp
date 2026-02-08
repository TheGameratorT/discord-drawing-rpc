#include "CropWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

namespace DiscordDrawRPC {

constexpr int BORDER_PADDING = 4;
constexpr int HANDLE_SIZE = 10;
constexpr int OVERLAY_ALPHA = 150;
constexpr int BORDER_WIDTH = 3;

const QString DISCORD_BLUE = "#5865F2";
const QString DARK_BG = "#2b2b2b";

CropWidget::CropWidget(QWidget* parent)
    : QLabel(parent)
    , m_dragging(false)
    , m_resizing(false)
    , m_resizeCorner(-1)
{
    setMinimumSize(400, 300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAlignment(Qt::AlignCenter);
    setStyleSheet(QString("border: 2px solid %1; background-color: %2; border-radius: 8px;").arg(DISCORD_BLUE, DARK_BG));
    setMouseTracking(true);
    setScaledContents(false);
}

QRect CropWidget::getImageDisplayBounds() const {
    if (m_displayPixmap.isNull()) {
        return QRect(0, 0, 0, 0);
    }
    
    int imgWidth = m_displayPixmap.width();
    int imgHeight = m_displayPixmap.height();
    int xOffset = (width() - imgWidth) / 2;
    int yOffset = (height() - imgHeight) / 2;
    
    return QRect(xOffset, yOffset, imgWidth, imgHeight);
}

int CropWidget::getCornerAtPos(const QPoint& pos) const {
    if (!cropRect.isValid()) {
        return -1;
    }
    
    QPoint corners[4] = {
        QPoint(cropRect.left(), cropRect.top()),     // 0: Top-left
        QPoint(cropRect.right(), cropRect.top()),    // 1: Top-right
        QPoint(cropRect.left(), cropRect.bottom()),  // 2: Bottom-left
        QPoint(cropRect.right(), cropRect.bottom())  // 3: Bottom-right
    };
    
    for (int i = 0; i < 4; ++i) {
        QRect handleRect(
            corners[i].x() - HANDLE_SIZE,
            corners[i].y() - HANDLE_SIZE,
            HANDLE_SIZE * 2,
            HANDLE_SIZE * 2
        );
        if (handleRect.contains(pos)) {
            return i;
        }
    }
    
    return -1;
}

void CropWidget::setImage(const QPixmap& pixmap) {
    m_originalPixmap = pixmap;
    
    // Account for border padding when scaling image
    QSize availableSize(width() - 2 * BORDER_PADDING, height() - 2 * BORDER_PADDING);
    
    // Scale image to fit widget while maintaining aspect ratio
    m_displayPixmap = pixmap.scaled(
        availableSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    
    // Calculate where the image is displayed
    QRect imgBounds = getImageDisplayBounds();
    
    // Create square crop rect - size is min of width and height
    int squareSize = qMin(imgBounds.width(), imgBounds.height());
    
    // Center the square on the image
    int cropX = imgBounds.x() + (imgBounds.width() - squareSize) / 2;
    int cropY = imgBounds.y() + (imgBounds.height() - squareSize) / 2;
    
    cropRect = QRect(cropX, cropY, squareSize, squareSize);
    
    // Store as ratio for resize responsiveness
    if (imgBounds.width() > 0 && imgBounds.height() > 0) {
        cropRectRatio.clear();
        cropRectRatio.append((cropX - imgBounds.x()) / (qreal)imgBounds.width());
        cropRectRatio.append((cropY - imgBounds.y()) / (qreal)imgBounds.height());
        cropRectRatio.append(squareSize / (qreal)imgBounds.width());
    }
    
    update();
}

QRect CropWidget::getCropRectOnOriginal() const {
    if (m_originalPixmap.isNull() || m_displayPixmap.isNull()) {
        return QRect();
    }
    
    // Calculate image display position
    QRect imgBounds = getImageDisplayBounds();
    
    // Convert crop rect from widget coordinates to display image coordinates
    int cropX = cropRect.x() - imgBounds.x();
    int cropY = cropRect.y() - imgBounds.y();
    int cropSize = cropRect.width();
    
    // Scale to original image coordinates
    qreal scaleX = m_originalPixmap.width() / (qreal)imgBounds.width();
    qreal scaleY = m_originalPixmap.height() / (qreal)imgBounds.height();
    
    int origX = static_cast<int>(cropX * scaleX);
    int origY = static_cast<int>(cropY * scaleY);
    int origSize = static_cast<int>(cropSize * scaleX);
    
    return QRect(origX, origY, origSize, origSize);
}

void CropWidget::paintEvent(QPaintEvent* event) {
    QLabel::paintEvent(event);
    
    if (m_displayPixmap.isNull()) {
        return;
    }
    
    QPainter painter(this);
    
    // Draw the image centered with padding for the border
    QRect imgBounds = getImageDisplayBounds();
    painter.drawPixmap(imgBounds.topLeft(), m_displayPixmap);
    
    // Draw semi-transparent overlay outside crop area
    QColor overlayColor(0, 0, 0, OVERLAY_ALPHA);
    
    // Top rectangle
    if (cropRect.top() > 0) {
        painter.fillRect(0, 0, width(), cropRect.top(), overlayColor);
    }
    
    // Bottom rectangle
    if (cropRect.bottom() < height()) {
        painter.fillRect(0, cropRect.bottom() + 1, width(), 
                        height() - cropRect.bottom() - 1, overlayColor);
    }
    
    // Left rectangle
    if (cropRect.left() > 0) {
        painter.fillRect(0, cropRect.top(), cropRect.left(), 
                        cropRect.height(), overlayColor);
    }
    
    // Right rectangle
    if (cropRect.right() < width()) {
        painter.fillRect(cropRect.right() + 1, cropRect.top(), 
                        width() - cropRect.right() - 1, 
                        cropRect.height(), overlayColor);
    }
    
    // Draw crop rectangle border
    QPen pen(QColor(DISCORD_BLUE), BORDER_WIDTH);
    painter.setPen(pen);
    painter.drawRect(cropRect);
    
    // Draw corner handles
    painter.setBrush(QColor(DISCORD_BLUE));
    QPoint corners[4] = {
        QPoint(cropRect.left(), cropRect.top()),
        QPoint(cropRect.right(), cropRect.top()),
        QPoint(cropRect.left(), cropRect.bottom()),
        QPoint(cropRect.right(), cropRect.bottom())
    };
    
    for (const QPoint& corner : corners) {
        painter.drawRect(corner.x() - HANDLE_SIZE/2, corner.y() - HANDLE_SIZE/2, 
                        HANDLE_SIZE, HANDLE_SIZE);
    }
}

void CropWidget::resizeEvent(QResizeEvent* event) {
    QLabel::resizeEvent(event);
    
    if (!m_originalPixmap.isNull() && cropRectRatio.size() == 3) {
        // Rescale the image to new size
        QSize availableSize(width() - 2 * BORDER_PADDING, height() - 2 * BORDER_PADDING);
        
        m_displayPixmap = m_originalPixmap.scaled(
            availableSize,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        
        // Recalculate crop rect based on stored ratio
        QRect imgBounds = getImageDisplayBounds();
        
        // Restore crop rect from ratio
        int cropX = imgBounds.x() + static_cast<int>(cropRectRatio[0] * imgBounds.width());
        int cropY = imgBounds.y() + static_cast<int>(cropRectRatio[1] * imgBounds.height());
        int squareSize = static_cast<int>(cropRectRatio[2] * imgBounds.width());
        
        cropRect = QRect(cropX, cropY, squareSize, squareSize);
        update();
    }
}

void CropWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // Check if clicking on a corner handle first
        int corner = getCornerAtPos(event->pos());
        if (corner >= 0) {
            m_resizing = true;
            m_resizeCorner = corner;
            m_dragStart = event->pos();
            m_resizeStartRect = cropRect;
        } else if (cropRect.contains(event->pos())) {
            m_dragging = true;
            m_dragStart = event->pos() - cropRect.topLeft();
        }
    }
}

void CropWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_resizing && !m_displayPixmap.isNull()) {
        // Get image bounds
        QRect imgBounds = getImageDisplayBounds();
        
        // Calculate delta from drag start
        QPoint delta = event->pos() - m_dragStart;
        
        // Start with the original rectangle
        QRect newRect = m_resizeStartRect;
        
        // Calculate new size based on which corner is being dragged
        // We need to maintain square aspect ratio
        int maxDelta;
        switch (m_resizeCorner) {
            case 0: // Top-left
                maxDelta = qMax(qAbs(delta.x()), qAbs(delta.y()));
                if (delta.x() > 0 || delta.y() > 0) {
                    maxDelta = qMin(delta.x(), delta.y());
                } else {
                    maxDelta = -maxDelta;
                }
                newRect.setLeft(m_resizeStartRect.left() + maxDelta);
                newRect.setTop(m_resizeStartRect.top() + maxDelta);
                break;
            case 1: // Top-right
                maxDelta = qMax(qAbs(delta.x()), qAbs(delta.y()));
                if (delta.x() < 0 || delta.y() > 0) {
                    maxDelta = -qMin(-delta.x(), delta.y());
                } else {
                    maxDelta = qMax(delta.x(), -delta.y());
                }
                newRect.setRight(m_resizeStartRect.right() + maxDelta);
                newRect.setTop(m_resizeStartRect.top() - maxDelta);
                break;
            case 2: // Bottom-left
                maxDelta = qMax(qAbs(delta.x()), qAbs(delta.y()));
                if (delta.x() > 0 || delta.y() < 0) {
                    maxDelta = -qMin(delta.x(), -delta.y());
                } else {
                    maxDelta = qMax(-delta.x(), delta.y());
                }
                newRect.setLeft(m_resizeStartRect.left() - maxDelta);
                newRect.setBottom(m_resizeStartRect.bottom() + maxDelta);
                break;
            case 3: // Bottom-right
                maxDelta = qMax(qAbs(delta.x()), qAbs(delta.y()));
                if (delta.x() < 0 || delta.y() < 0) {
                    maxDelta = qMin(delta.x(), delta.y());
                } else {
                    maxDelta = maxDelta;
                }
                newRect.setRight(m_resizeStartRect.right() + maxDelta);
                newRect.setBottom(m_resizeStartRect.bottom() + maxDelta);
                break;
        }
        
        // Constrain to image bounds and minimum size
        const int minSize = 50;
        if (newRect.width() >= minSize && newRect.height() >= minSize &&
            newRect.left() >= imgBounds.left() && newRect.top() >= imgBounds.top() &&
            newRect.right() <= imgBounds.right() && newRect.bottom() <= imgBounds.bottom()) {
            
            cropRect = newRect;
            
            // Update ratio
            if (imgBounds.width() > 0 && imgBounds.height() > 0) {
                cropRectRatio.clear();
                cropRectRatio.append((cropRect.left() - imgBounds.x()) / (qreal)imgBounds.width());
                cropRectRatio.append((cropRect.top() - imgBounds.y()) / (qreal)imgBounds.height());
                cropRectRatio.append(cropRect.width() / (qreal)imgBounds.width());
            }
            
            update();
        }
    } else if (m_dragging && !m_displayPixmap.isNull()) {
        // Calculate new position
        QPoint newPos = event->pos() - m_dragStart;
        
        // Get image bounds
        QRect imgBounds = getImageDisplayBounds();
        
        // Constrain to image bounds
        int newX = qMax(imgBounds.x(), qMin(newPos.x(), imgBounds.right() - cropRect.width()));
        int newY = qMax(imgBounds.y(), qMin(newPos.y(), imgBounds.bottom() - cropRect.height()));
        
        cropRect.moveTo(newX, newY);
        
        // Update ratio to maintain position on resize
        if (imgBounds.width() > 0 && imgBounds.height() > 0) {
            cropRectRatio.clear();
            cropRectRatio.append((newX - imgBounds.x()) / (qreal)imgBounds.width());
            cropRectRatio.append((newY - imgBounds.y()) / (qreal)imgBounds.height());
            cropRectRatio.append(cropRect.width() / (qreal)imgBounds.width());
        }
        
        update();
    } else {
        // Change cursor based on what's under the mouse
        int corner = getCornerAtPos(event->pos());
        if (corner >= 0) {
            // Set resize cursor based on corner
            if (corner == 0 || corner == 3) { // TL or BR
                setCursor(Qt::SizeFDiagCursor);
            } else { // TR or BL
                setCursor(Qt::SizeBDiagCursor);
            }
        } else if (cropRect.contains(event->pos())) {
            setCursor(Qt::SizeAllCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }
}

void CropWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        m_resizing = false;
        m_resizeCorner = -1;
    }
}

} // namespace DiscordDrawRPC
