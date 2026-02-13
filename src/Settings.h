#pragma once

#include <QObject>
#include <QString>
#include <QKeySequence>

class Settings : public QObject {
    Q_OBJECT
public:
    enum class Backend {
        OpenAI,
        Gemini
    };
    Q_ENUM(Backend)

    explicit Settings(QObject *parent = nullptr);

    // API Keys
    QString apiKey(Backend backend) const;
    void setApiKey(Backend backend, const QString &key);

    // Target language
    QString targetLanguage() const;
    void setTargetLanguage(const QString &lang);

    // Active backend
    Backend activeBackend() const;
    void setActiveBackend(Backend backend);

    // Hotkey
    QKeySequence hotkey() const;
    void setHotkey(const QKeySequence &key);

    // Overlay font size
    int overlayFontSize() const;
    void setOverlayFontSize(int size);

signals:
    void settingsChanged();

private:
    static QString backendKey(Backend backend);
};
