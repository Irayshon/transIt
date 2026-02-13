#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QRect>
#include <QPropertyAnimation>
#include <QTimer>

class OverlayWindow : public QWidget {
    Q_OBJECT
public:
    explicit OverlayWindow(QWidget *parent = nullptr);

    void showLoading(const QRect &selectionRect);
    void showResult(const QString &text);
    void showError(const QString &error);
    void dismiss();

    void setFontSize(int size);

signals:
    void dismissed();
    void copyRequested();
    void saveRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;

private:
    void setupUi();
    void adjustSizeToContent();

    QLabel *m_textLabel = nullptr;
    QLabel *m_loadingLabel = nullptr;
    QPushButton *m_copyBtn = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QWidget *m_buttonBar = nullptr;

    QRect m_selectionRect;
    QString m_currentText;
    int m_fontSize = 14;

    static constexpr int PADDING = 12;
    static constexpr int BUTTON_BAR_HEIGHT = 36;
};
