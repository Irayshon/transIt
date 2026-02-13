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
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(PADDING, PADDING, PADDING, PADDING);
    mainLayout->setSpacing(8);

    // Loading label
    m_loadingLabel = new QLabel("Translating...", this);
    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->setStyleSheet("color: #cccccc; font-size: 14px;");

    // Result text label
    m_textLabel = new QLabel(this);
    m_textLabel->setWordWrap(true);
    m_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_textLabel->setStyleSheet(
        QString("color: #ffffff; font-size: %1px; line-height: 1.4;")
            .arg(m_fontSize));
    m_textLabel->hide();

    // Button bar
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

    mainLayout->addWidget(m_loadingLabel);
    mainLayout->addWidget(m_textLabel, 1);
    mainLayout->addWidget(m_buttonBar);

    // Connections
    connect(m_copyBtn, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_currentText);
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
                out << m_currentText;
            }
        }
    });

    connect(m_closeBtn, &QPushButton::clicked, this, &OverlayWindow::dismiss);
}

void OverlayWindow::showLoading(const QRect &selectionRect) {
    m_selectionRect = selectionRect;
    m_loadingLabel->show();
    m_textLabel->hide();
    m_buttonBar->hide();

    setGeometry(selectionRect);
    show();
    activateWindow();
}

void OverlayWindow::showResult(const QString &text) {
    m_currentText = text;
    m_loadingLabel->hide();
    m_textLabel->setText(text);
    m_textLabel->show();
    m_buttonBar->show();

    adjustSizeToContent();
}

void OverlayWindow::showError(const QString &error) {
    m_loadingLabel->hide();
    m_textLabel->setText("Error: " + error);
    m_textLabel->setStyleSheet(
        QString("color: #ff6b6b; font-size: %1px;").arg(m_fontSize));
    m_textLabel->show();
    m_buttonBar->show();
    m_copyBtn->hide();
    m_saveBtn->hide();

    adjustSizeToContent();
}

void OverlayWindow::dismiss() {
    hide();
    emit dismissed();
}

void OverlayWindow::setFontSize(int size) {
    m_fontSize = size;
    m_textLabel->setStyleSheet(
        QString("color: #ffffff; font-size: %1px; line-height: 1.4;")
            .arg(m_fontSize));
}

void OverlayWindow::adjustSizeToContent() {
    QFontMetrics fm(m_textLabel->font());
    int availableWidth = m_selectionRect.width() - 2 * PADDING;
    QRect textRect = fm.boundingRect(
        QRect(0, 0, availableWidth, 99999),
        Qt::TextWordWrap, m_currentText);

    int neededHeight = textRect.height() + BUTTON_BAR_HEIGHT + 3 * PADDING;
    int newHeight = qMax(m_selectionRect.height(), neededHeight);

    // Clamp to screen bottom
    QScreen *screen = QGuiApplication::screenAt(m_selectionRect.center());
    if (!screen) screen = QGuiApplication::primaryScreen();
    int maxHeight = screen->availableGeometry().bottom() - m_selectionRect.y();
    newHeight = qMin(newHeight, maxHeight);

    setGeometry(m_selectionRect.x(), m_selectionRect.y(),
                m_selectionRect.width(), newHeight);
}

void OverlayWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        dismiss();
    } else if (event->matches(QKeySequence::Copy)) {
        QApplication::clipboard()->setText(m_currentText);
    }
    QWidget::keyPressEvent(event);
}

void OverlayWindow::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Dark semi-transparent background with rounded corners
    painter.setBrush(QColor(30, 30, 30, 220));
    painter.setPen(QColor(0, 120, 215)); // Windows 11 accent blue border
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 8, 8);
}

bool OverlayWindow::event(QEvent *event) {
    if (event->type() == QEvent::WindowDeactivate) {
        // Dismiss when clicking outside
        dismiss();
        return true;
    }
    return QWidget::event(event);
}
