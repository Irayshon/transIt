#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QRect>
#include <QVector>

#include "AIService.h"

class OverlayWindow : public QWidget {
    Q_OBJECT
public:
    explicit OverlayWindow(QWidget *parent = nullptr);

    void showLoading(const QRect &selectionRect);
    void showResult(const QVector<TextBlock> &blocks);
    void showError(const QString &error);
    void dismiss();

    void setFontSize(int size);

signals:
    void dismissed();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;

private:
    void setupUi();
    bool blocksOverflow() const;
    void adjustSizeForFallback();

    QLabel *m_loadingLabel = nullptr;
    QPushButton *m_copyBtn = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QWidget *m_buttonBar = nullptr;

    QRect m_selectionRect;
    QVector<TextBlock> m_blocks;
    QString m_plainText;
    int m_fontSize = 14;
    bool m_showBlocks = false;
    bool m_usePositionedLayout = false;
    bool m_hasError = false;
    QString m_errorText;

    static constexpr int PADDING = 12;
    static constexpr int BUTTON_BAR_HEIGHT = 36;
    static constexpr int MAX_FONT_PX = 48;
};
