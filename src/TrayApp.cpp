#include "TrayApp.h"
#include "OpenAIBackend.h"
#include "GeminiBackend.h"

#include <QApplication>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QKeySequenceEdit>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QBuffer>
#include <QMessageBox>
#include <QIcon>

TrayApp::TrayApp(QObject *parent)
    : QObject(parent)
{
    m_settings = new Settings(this);
    m_hotkeyManager = new HotkeyManager(this);
    m_regionSelector = new RegionSelector();
    m_overlayWindow = new OverlayWindow();
}

TrayApp::~TrayApp() {
    if (m_trayIcon)
        m_trayIcon->setContextMenu(nullptr);
    delete m_trayMenu;
    delete m_regionSelector;
    delete m_overlayWindow;
}

void TrayApp::initialize() {
    createTrayIcon();
    createAIService();
    registerHotkey();

    // Connections
    connect(m_hotkeyManager, &HotkeyManager::hotkeyTriggered,
            this, &TrayApp::onHotkeyTriggered);
    connect(m_regionSelector, &RegionSelector::regionSelected,
            this, &TrayApp::onRegionSelected);

    connect(m_overlayWindow, &OverlayWindow::dismissed,
            this, [this]() {
                if (m_aiService) m_aiService->cancel();
            });

    m_overlayWindow->setFontSize(m_settings->overlayFontSize());
}

void TrayApp::onHotkeyTriggered() {
    m_overlayWindow->dismiss();
    m_regionSelector->start();
}

void TrayApp::onRegionSelected(const QRect &region, const QPixmap &screenshot) {
    m_overlayWindow->showLoading(region);

    // Encode screenshot to PNG bytes
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    screenshot.save(&buffer, "PNG");
    buffer.close();

    // Ensure AI service is current
    createAIService();

    if (!m_aiService) {
        m_overlayWindow->showError("No API key configured. Right-click tray icon → Settings.");
        return;
    }

    m_aiService->translate(imageData, m_settings->targetLanguage());
}

void TrayApp::onTranslationReady(const QVector<TextBlock> &blocks) {
    m_overlayWindow->showResult(blocks);
}

void TrayApp::onTranslationFailed(const QString &error) {
    m_overlayWindow->showError(error);
}

void TrayApp::createTrayIcon() {
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setToolTip("TransIt - Screen Translator");

    m_trayIcon->setIcon(QIcon(":/icons/tray.png"));

    m_trayMenu = new QMenu();

    QAction *settingsAction = m_trayMenu->addAction("Settings...");
    connect(settingsAction, &QAction::triggered, this, &TrayApp::showSettingsDialog);

    m_trayMenu->addSeparator();

    QAction *quitAction = m_trayMenu->addAction("Quit");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();

    m_trayIcon->showMessage("TransIt", "Running in background. Press "
        + m_settings->hotkey().toString() + " to translate.",
        QSystemTrayIcon::Information, 3000);
}

void TrayApp::createAIService() {
    if (m_aiService) {
        m_aiService->cancel();
        disconnect(m_aiService, nullptr, this, nullptr);
        m_aiService->deleteLater();
        m_aiService = nullptr;
    }

    Settings::Backend backend = m_settings->activeBackend();
    QString apiKey = m_settings->apiKey(backend);

    if (apiKey.isEmpty())
        return;

    QString baseUrl = m_settings->baseUrl(backend);
    QString modelName = m_settings->modelName(backend);

    switch (backend) {
        case Settings::Backend::OpenAI:
            m_aiService = new OpenAIBackend(apiKey, baseUrl, modelName, this);
            break;
        case Settings::Backend::Gemini:
            m_aiService = new GeminiBackend(apiKey, baseUrl, modelName, this);
            break;
    }

    if (m_aiService) {
        connect(m_aiService, &AIService::translationReady,
                this, &TrayApp::onTranslationReady);
        connect(m_aiService, &AIService::translationFailed,
                this, &TrayApp::onTranslationFailed);
    }
}

void TrayApp::registerHotkey() {
    if (!m_hotkeyManager->registerHotkey(m_settings->hotkey())) {
        // Hotkey registration may fail on Linux dev environment — that's OK
        qWarning("Failed to register hotkey. This is expected on non-Windows.");
    }
}

void TrayApp::showSettingsDialog() {
    QDialog dialog;
    dialog.setWindowTitle("TransIt Settings");
    dialog.setMinimumWidth(400);

    auto *layout = new QFormLayout(&dialog);

    // Backend selection
    auto *backendCombo = new QComboBox();
    backendCombo->addItem("OpenAI-Compatible", static_cast<int>(Settings::Backend::OpenAI));
    backendCombo->addItem("Google Gemini", static_cast<int>(Settings::Backend::Gemini));
    backendCombo->setCurrentIndex(static_cast<int>(m_settings->activeBackend()));
    layout->addRow("AI Backend:", backendCombo);

    auto *openaiUrlEdit = new QLineEdit(m_settings->baseUrl(Settings::Backend::OpenAI));
    openaiUrlEdit->setPlaceholderText("https://api.openai.com");
    layout->addRow("OpenAI Base URL:", openaiUrlEdit);

    auto *openaiModelEdit = new QLineEdit(m_settings->modelName(Settings::Backend::OpenAI));
    openaiModelEdit->setPlaceholderText("gpt-4o");
    layout->addRow("OpenAI Model:", openaiModelEdit);

    auto *openaiKeyEdit = new QLineEdit(m_settings->apiKey(Settings::Backend::OpenAI));
    openaiKeyEdit->setEchoMode(QLineEdit::Password);
    openaiKeyEdit->setPlaceholderText("sk-...");
    layout->addRow("OpenAI API Key:", openaiKeyEdit);

    auto *geminiUrlEdit = new QLineEdit(m_settings->baseUrl(Settings::Backend::Gemini));
    geminiUrlEdit->setPlaceholderText("https://generativelanguage.googleapis.com");
    layout->addRow("Gemini Base URL:", geminiUrlEdit);

    auto *geminiModelEdit = new QLineEdit(m_settings->modelName(Settings::Backend::Gemini));
    geminiModelEdit->setPlaceholderText("gemini-2.0-flash");
    layout->addRow("Gemini Model:", geminiModelEdit);

    auto *geminiKeyEdit = new QLineEdit(m_settings->apiKey(Settings::Backend::Gemini));
    geminiKeyEdit->setEchoMode(QLineEdit::Password);
    geminiKeyEdit->setPlaceholderText("AI...");
    layout->addRow("Gemini API Key:", geminiKeyEdit);

    // Target language
    auto *langCombo = new QComboBox();
    langCombo->setEditable(true);
    QStringList languages = {
        "English", "Chinese (Simplified)", "Chinese (Traditional)",
        "Japanese", "Korean", "Spanish", "French", "German",
        "Russian", "Portuguese", "Arabic", "Hindi"
    };
    langCombo->addItems(languages);
    langCombo->setCurrentText(m_settings->targetLanguage());
    layout->addRow("Target Language:", langCombo);

    // Hotkey
    auto *hotkeyEdit = new QKeySequenceEdit(m_settings->hotkey());
    layout->addRow("Hotkey:", hotkeyEdit);

    // Font size
    auto *fontSizeSpin = new QSpinBox();
    fontSizeSpin->setRange(8, 32);
    fontSizeSpin->setValue(m_settings->overlayFontSize());
    layout->addRow("Overlay Font Size:", fontSizeSpin);

    // OK / Cancel
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        m_settings->setActiveBackend(
            static_cast<Settings::Backend>(backendCombo->currentData().toInt()));
        m_settings->setBaseUrl(Settings::Backend::OpenAI, openaiUrlEdit->text());
        m_settings->setModelName(Settings::Backend::OpenAI, openaiModelEdit->text());
        m_settings->setApiKey(Settings::Backend::OpenAI, openaiKeyEdit->text());
        m_settings->setBaseUrl(Settings::Backend::Gemini, geminiUrlEdit->text());
        m_settings->setModelName(Settings::Backend::Gemini, geminiModelEdit->text());
        m_settings->setApiKey(Settings::Backend::Gemini, geminiKeyEdit->text());
        m_settings->setTargetLanguage(langCombo->currentText());
        m_settings->setOverlayFontSize(fontSizeSpin->value());

        // Re-register hotkey if changed
        QKeySequence newHotkey = hotkeyEdit->keySequence();
        if (newHotkey != m_settings->hotkey()) {
            m_settings->setHotkey(newHotkey);
            registerHotkey();
        }

        m_overlayWindow->setFontSize(m_settings->overlayFontSize());
        createAIService();
    }
}
