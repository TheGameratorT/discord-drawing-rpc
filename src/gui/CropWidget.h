#pragma once

#include <QLabel>
#include <QPixmap>
#include <QRect>
#include <QVector>

namespace DiscordDrawRPC {

class CropWidget : public QLabel {
    Q_OBJECT
    
public:
    explicit CropWidget(QWidget* parent = nullptr);
    
    void setImage(const QPixmap& pixmap);
    QRect getCropRectOnOriginal() const;
    QRect getImageDisplayBounds() const;
    
    QVector<qreal> cropRectRatio;
    QRect cropRect;
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    
private:
    int getCornerAtPos(const QPoint& pos) const;
    
    QPixmap m_originalPixmap;
    QPixmap m_displayPixmap;
    
    bool m_dragging;
    bool m_resizing;
    int m_resizeCorner;
    QPoint m_dragStart;
    QRect m_resizeStartRect;
};

} // namespace DiscordDrawRPC
