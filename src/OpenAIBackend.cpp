#include "OpenAIBackend.h"

#include <QtConcurrent>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

OpenAIBackend::OpenAIBackend(const QString &apiKey, const QString &baseUrl,
                             const QString &modelName, QObject *parent)
    : AIService(parent), m_apiKey(apiKey), m_baseUrl(baseUrl), m_modelName(modelName) {}

OpenAIBackend::~OpenAIBackend() {
    m_cancelled = true;
    // WARNING: Do not add waitForFinished() — blocks GUI thread up to 30s.
    // QPointer guards in the lambda handle safe destruction.
}

void OpenAIBackend::translate(const QByteArray &pngImageData,
                               const QString &targetLanguage) {
    m_cancelled = false;

    QString apiKey = m_apiKey;
    QString baseUrl = m_baseUrl;
    QString modelName = m_modelName;
    QString lang = targetLanguage;
    QByteArray imageData = pngImageData;
    QPointer<OpenAIBackend> self(this);

    m_future = QtConcurrent::run([self, apiKey, baseUrl, modelName, lang, imageData]() {
        try {
            QString base64Image = QString::fromLatin1(imageData.toBase64());
            QString dataUrl = "data:image/png;base64," + base64Image;

            QString prompt = QString(
                "OCR the text in this image and translate it to %1. "
                "Return ONLY the translated text, nothing else. "
                "If no text is found, respond with '[No text detected]'."
            ).arg(lang);

            json payload = {
                {"model", modelName.toStdString()},
                {"messages", {{
                    {"role", "user"},
                    {"content", {
                        {{"type", "text"}, {"text", prompt.toStdString()}},
                        {{"type", "image_url"}, {"image_url", {{"url", dataUrl.toStdString()}}}}
                    }}
                }}},
                {"max_tokens", 4096}
            };

            // Normalize base URL: strip trailing slash and /v1 to avoid duplication
            QString normalizedUrl = baseUrl;
            while (normalizedUrl.endsWith('/'))
                normalizedUrl.chop(1);
            if (normalizedUrl.endsWith("/v1"))
                normalizedUrl.chop(3);

            QString endpoint = normalizedUrl + "/v1/chat/completions";

            cpr::Response response = cpr::Post(
                cpr::Url{endpoint.toStdString()},
                cpr::Header{
                    {"Content-Type", "application/json"},
                    {"Authorization", "Bearer " + apiKey.toStdString()}
                },
                cpr::Body{payload.dump()},
                cpr::Timeout{30000}
            );

            if (!self || self->m_cancelled) return;

            if (response.status_code != 200) {
                QString error = QString("API error (HTTP %1): %2")
                    .arg(response.status_code)
                    .arg(QString::fromStdString(response.text).left(200));
                if (!self) return;
                QMetaObject::invokeMethod(self.data(), [self, error]() {
                    if (self) emit self->translationFailed(error);
                }, Qt::QueuedConnection);
                return;
            }

            json result = json::parse(response.text);
            std::string text = result["choices"][0]["message"]["content"];
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

void OpenAIBackend::cancel() {
    m_cancelled = true;
}
