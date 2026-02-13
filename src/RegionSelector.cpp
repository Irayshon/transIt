#include "RegionSelector.h"

#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QKeyEvent>

RegionSelector::RegionSelector(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setCursor(Qt::CrossCursor);
}

void RegionSelector::start() {
    QRect fullGeometry;
    for (QScreen *screen : QGuiApplication::screens()) {
        fullGeometry = fullGeometry.united(screen->geometry());
    }
    setGeometry(fullGeometry);

    m_selecting = false;
    m_startPos = QPoint();
    m_currentPos = QPoint();

    showFullScreen();
    activateWindow();
    grabMouse();
    grabKeyboard();
}

void RegionSelector::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    painter.fillRect(rect(), QColor(0, 0, 0, 100));

    if (m_selecting) {
        QRect selection = QRect(m_startPos, m_currentPos).normalized();

        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(selection, Qt::transparent);

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        QPen pen(QColor(0, 120, 215), 2);
        painter.setPen(pen);
        painter.drawRect(selection);
    }
}

void RegionSelector::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_startPos = event->globalPosition().toPoint();
        m_currentPos = m_startPos;
        m_selecting = true;
        update();
    }
}

void RegionSelector::mouseMoveEvent(QMouseEvent *event) {
    if (m_selecting) {
        m_currentPos = event->globalPosition().toPoint();
        update();
    }
}

void RegionSelector::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_selecting) {
        m_selecting = false;
        releaseMouse();
        releaseKeyboard();
        hide();

        QRect region = QRect(m_startPos, event->globalPosition().toPoint()).normalized();

        if (region.width() < 10 || region.height() < 10) {
            emit selectionCancelled();
            return;
        }

        QPixmap screenshot = captureRegion(region);
        emit regionSelected(region, screenshot);
    }
}

void RegionSelector::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        m_selecting = false;
        releaseMouse();
        releaseKeyboard();
        hide();
        emit selectionCancelled();
    }
}

QPixmap RegionSelector::captureRegion(const QRect &region) {
    QScreen *screen = QGuiApplication::screenAt(region.center());
    if (!screen)
        screen = QGuiApplication::primaryScreen();

    return screen->grabWindow(0,
        region.x() - screen->geometry().x(),
        region.y() - screen->geometry().y(),
        region.width(),
        region.height());
}
