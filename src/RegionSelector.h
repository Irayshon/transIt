#pragma once

#include <QWidget>
#include <QPixmap>
#include <QRect>
#include <QPoint>

class RegionSelector : public QWidget {
    Q_OBJECT
public:
    explicit RegionSelector(QWidget *parent = nullptr);

    void start();

signals:
    void regionSelected(const QRect &region, const QPixmap &screenshot);
    void selectionCancelled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QPixmap captureRegion(const QRect &region);

    QPoint m_startPos;
    QPoint m_currentPos;
    bool m_selecting = false;
};
