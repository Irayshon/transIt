#include "OverlayWindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPainter>
#include <QClipboard>
#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <QFileDialog>
#include <QTextStream>
#include <QFontMetrics>
#include <QFile>
#include <QTimer>

OverlayWindow::OverlayWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                   | Qt::Tool | Qt::BypassWindowManagerHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating, false);

    setupUi();
}

void OverlayWindow::setupUi() {
    m_loadingLabel = new QLabel("Translating...", this);
    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->setStyleSheet("color: #cccccc; font-size: 14px;");

    m_buttonBar = new QWidget(this);
    auto *btnLayout = new QHBoxLayout(m_buttonBar);
    btnLayout->setContentsMargins(0, 4, 0, 0);
    btnLayout->setSpacing(8);

    m_copyBtn = new QPushButton("Copy", m_buttonBar);
    m_saveBtn = new QPushButton("Save", m_buttonBar);
    m_closeBtn = new QPushButton("Close", m_buttonBar);

    QString btnStyle =
        "QPushButton {"
        "  background: rgba(255,255,255,0.15);"
        "  color: #ffffff;"
        "  border: 1px solid rgba(255,255,255,0.3);"
        "  border-radius: 4px;"
        "  padding: 4px 12px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(255,255,255,0.25);"
        "}";
    m_copyBtn->setStyleSheet(btnStyle);
    m_saveBtn->setStyleSheet(btnStyle);
    m_closeBtn->setStyleSheet(btnStyle);

    btnLayout->addStretch();
    btnLayout->addWidget(m_copyBtn);
    btnLayout->addWidget(m_saveBtn);
    btnLayout->addWidget(m_closeBtn);
    m_buttonBar->hide();

    connect(m_copyBtn, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_plainText);
        m_copyBtn->setText("Copied!");
        QTimer::singleShot(1500, this, [this]() {
            m_copyBtn->setText("Copy");
        });
    });

    connect(m_saveBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getSaveFileName(
            this, "Save Translation", "translation.txt",
            "Text Files (*.txt);;All Files (*)");
        if (!path.isEmpty()) {
            QFile file(path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << m_plainText;
            }
        }
    });

    connect(m_closeBtn, &QPushButton::clicked, this, &OverlayWindow::dismiss);
}

void OverlayWindow::showLoading(const QRect &selectionRect) {
    m_selectionRect = selectionRect;
    m_blocks.clear();
    m_plainText.clear();
    m_showBlocks = false;
    m_hasError = false;
    m_errorText.clear();

    m_loadingLabel->show();
    m_loadingLabel->setGeometry(0, 0, selectionRect.width(), selectionRect.height());
    m_buttonBar->hide();

    setGeometry(selectionRect);
    show();
    activateWindow();
}

void OverlayWindow::showResult(const QVector<TextBlock> &blocks) {
    m_blocks = blocks;
    m_showBlocks = true;
    m_hasError = false;
    m_loadingLabel->hide();

    QStringList lines;
    for (const auto &b : blocks)
        lines.append(b.text);
    m_plainText = lines.join('\n');

    m_copyBtn->show();
    m_saveBtn->show();
    m_buttonBar->show();
    m_buttonBar->setGeometry(PADDING, m_selectionRect.height() - BUTTON_BAR_HEIGHT - PADDING,
                             m_selectionRect.width() - 2 * PADDING, BUTTON_BAR_HEIGHT);

    update();
}

void OverlayWindow::showError(const QString &error) {
    m_hasError = true;
    m_errorText = error;
    m_showBlocks = false;
    m_loadingLabel->hide();

    m_buttonBar->show();
    m_copyBtn->hide();
    m_saveBtn->hide();
    m_buttonBar->setGeometry(PADDING, m_selectionRect.height() - BUTTON_BAR_HEIGHT - PADDING,
                             m_selectionRect.width() - 2 * PADDING, BUTTON_BAR_HEIGHT);

    update();
}

void OverlayWindow::dismiss() {
    hide();
    emit dismissed();
}

void OverlayWindow::setFontSize(int size) {
    m_fontSize = size;
}

void OverlayWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        dismiss();
    } else if (event->matches(QKeySequence::Copy)) {
        QApplication::clipboard()->setText(m_plainText);
    }
    QWidget::keyPressEvent(event);
}

void OverlayWindow::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setBrush(QColor(30, 30, 30, 220));
    painter.setPen(QColor(0, 120, 215));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 8, 8);

    if (m_hasError) {
        QFont font;
        font.setPixelSize(m_fontSize);
        painter.setFont(font);
        painter.setPen(QColor(255, 107, 107));
        painter.drawText(rect().adjusted(PADDING, PADDING, -PADDING, -BUTTON_BAR_HEIGHT - PADDING),
                         Qt::AlignCenter | Qt::TextWordWrap, "Error: " + m_errorText);
        return;
    }

    if (!m_showBlocks || m_blocks.isEmpty())
        return;

    int overlayW = width();
    int overlayH = height();

    for (const auto &block : m_blocks) {
        int bx = static_cast<int>(block.bbox.x() * overlayW);
        int by = static_cast<int>(block.bbox.y() * overlayH);
        int bw = static_cast<int>(block.bbox.width() * overlayW);
        int bh = static_cast<int>(block.bbox.height() * overlayH);

        QRect blockRect(bx, by, qMax(bw, 20), qMax(bh, 16));

        // Auto-fit font: start from bounding box height, shrink to fit width
        QFont font;
        int fontSize = qMax(bh, 10);
        font.setPixelSize(fontSize);
        QFontMetrics fm(font);
        while (fontSize > 8 && fm.horizontalAdvance(block.text) > blockRect.width()) {
            fontSize--;
            font.setPixelSize(fontSize);
            fm = QFontMetrics(font);
        }

        painter.setBrush(QColor(30, 30, 30, 180));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(blockRect.adjusted(-2, -1, 2, 1), 3, 3);

        painter.setFont(font);
        painter.setPen(Qt::white);
        painter.drawText(blockRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, block.text);
    }
}

bool OverlayWindow::event(QEvent *event) {
    if (event->type() == QEvent::WindowDeactivate) {
        dismiss();
        return true;
    }
    return QWidget::event(event);
}
