#include "GeminiBackend.h"

#include <QtConcurrent>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

GeminiBackend::GeminiBackend(const QString &apiKey, QObject *parent)
    : AIService(parent), m_apiKey(apiKey) {}

GeminiBackend::~GeminiBackend() {
    m_cancelled = true;
    // WARNING: Do not add waitForFinished() — blocks GUI thread up to 30s.
    // QPointer guards in the lambda handle safe destruction.
}

void GeminiBackend::translate(const QByteArray &pngImageData,
                               const QString &targetLanguage) {
    m_cancelled = false;

    QString apiKey = m_apiKey;
    QString lang = targetLanguage;
    QByteArray imageData = pngImageData;
    QPointer<GeminiBackend> self(this);

    m_future = QtConcurrent::run([self, apiKey, lang, imageData]() {
        try {
            QString base64Image = QString::fromLatin1(imageData.toBase64());

            QString prompt = QString(
                "OCR the text in this image and translate it to %1. "
                "Return ONLY the translated text, nothing else. "
                "If no text is found, respond with '[No text detected]'."
            ).arg(lang);

            json payload = {
                {"contents", {{
                    {"parts", {
                        {{"text", prompt.toStdString()}},
                        {{"inlineData", {
                            {"mimeType", "image/png"},
                            {"data", base64Image.toStdString()}
                        }}}
                    }}
                }}},
                {"generationConfig", {
                    {"maxOutputTokens", 4096}
                }}
            };

            QString url = QString(
                "https://generativelanguage.googleapis.com/v1beta/models/"
                "gemini-2.0-flash:generateContent?key=%1"
            ).arg(apiKey);

            cpr::Response response = cpr::Post(
                cpr::Url{url.toStdString()},
                cpr::Header{{"Content-Type", "application/json"}},
                cpr::Body{payload.dump()},
                cpr::Timeout{30000}
            );

            if (!self || self->m_cancelled) return;

            if (response.status_code != 200) {
                QString error = QString("Gemini API error (HTTP %1): %2")
                    .arg(response.status_code)
                    .arg(QString::fromStdString(response.text).left(200));
                if (!self) return;
                QMetaObject::invokeMethod(self.data(), [self, error]() {
                    if (self) emit self->translationFailed(error);
                }, Qt::QueuedConnection);
                return;
            }

            json result = json::parse(response.text);
            std::string text = result["candidates"][0]["content"]["parts"][0]["text"];
            QString translated = QString::fromStdString(text).trimmed();

            if (!self) return;
            QMetaObject::invokeMethod(self.data(), [self, translated]() {
                if (self) emit self->translationReady(translated);
            }, Qt::QueuedConnection);

        } catch (const std::exception &e) {
            if (!self || self->m_cancelled) return;
            QString error = QString("Request failed: %1").arg(e.what());
            if (!self) return;
            QMetaObject::invokeMethod(self.data(), [self, error]() {
                if (self) emit self->translationFailed(error);
            }, Qt::QueuedConnection);
        }
    });
}

void GeminiBackend::cancel() {
    m_cancelled = true;
}
