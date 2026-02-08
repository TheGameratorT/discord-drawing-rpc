#include "ScreenshotSelector.h"
#include <QPainter>
#include <QMouseEvent>

namespace DiscordDrawRPC {

ScreenshotSelector::ScreenshotSelector(const QPixmap& screenshot, QWidget* parent)
    : QWidget(parent)
    , m_screenshot(screenshot)
    , m_rubberBand(new QRubberBand(QRubberBand::Rectangle, this))
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setWindowState(Qt::WindowFullScreen);
    setStyleSheet("background-color: rgba(0, 0, 0, 100);");
}

QRect ScreenshotSelector::getRect() const {
    return QRect(m_begin, m_end).normalized();
}

void ScreenshotSelector::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.drawPixmap(rect(), m_screenshot);
}

void ScreenshotSelector::mousePressEvent(QMouseEvent* event) {
    m_begin = event->pos();
    m_end = m_begin;
    m_rubberBand->setGeometry(QRect(m_begin, QSize()));
    m_rubberBand->show();
}

void ScreenshotSelector::mouseMoveEvent(QMouseEvent* event) {
    m_end = event->pos();
    m_rubberBand->setGeometry(QRect(m_begin, m_end).normalized());
}

void ScreenshotSelector::mouseReleaseEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    m_rubberBand->hide();
    close();
}

} // namespace DiscordDrawRPC
