#pragma once

#include <QWidget>
#include <QPixmap>
#include <QRubberBand>
#include <QRect>

namespace DiscordDrawRPC {

class ScreenshotSelector : public QWidget {
    Q_OBJECT
    
public:
    explicit ScreenshotSelector(const QPixmap& screenshot, QWidget* parent = nullptr);
    
    QRect getRect() const;
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    
private:
    QPixmap m_screenshot;
    QPoint m_begin;
    QPoint m_end;
    QRubberBand* m_rubberBand;
};

} // namespace DiscordDrawRPC
