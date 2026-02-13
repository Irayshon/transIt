#include "Settings.h"
#include <QSettings>

Settings::Settings(QObject *parent)
    : QObject(parent) {}

QString Settings::apiKey(Backend backend) const {
    QSettings s;
    return s.value("api_keys/" + backendKey(backend)).toString();
}

void Settings::setApiKey(Backend backend, const QString &key) {
    QSettings s;
    s.setValue("api_keys/" + backendKey(backend), key);
    emit settingsChanged();
}

QString Settings::targetLanguage() const {
    QSettings s;
    return s.value("target_language", "English").toString();
}

void Settings::setTargetLanguage(const QString &lang) {
    QSettings s;
    s.setValue("target_language", lang);
    emit settingsChanged();
}

Settings::Backend Settings::activeBackend() const {
    QSettings s;
    return static_cast<Backend>(s.value("active_backend", 0).toInt());
}

void Settings::setActiveBackend(Backend backend) {
    QSettings s;
    s.setValue("active_backend", static_cast<int>(backend));
    emit settingsChanged();
}

QKeySequence Settings::hotkey() const {
    QSettings s;
    return QKeySequence(s.value("hotkey", "Ctrl+Alt+T").toString());
}

void Settings::setHotkey(const QKeySequence &key) {
    QSettings s;
    s.setValue("hotkey", key.toString());
    emit settingsChanged();
}

int Settings::overlayFontSize() const {
    QSettings s;
    return s.value("overlay_font_size", 14).toInt();
}

void Settings::setOverlayFontSize(int size) {
    QSettings s;
    s.setValue("overlay_font_size", size);
    emit settingsChanged();
}

QString Settings::backendKey(Backend backend) {
    switch (backend) {
        case Backend::OpenAI: return "openai";
        case Backend::Gemini: return "gemini";
    }
    return "unknown";
}
