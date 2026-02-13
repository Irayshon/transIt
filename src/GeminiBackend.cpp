#include "GeminiBackend.h"

#include <QtConcurrent>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

GeminiBackend::GeminiBackend(const QString &apiKey, const QString &baseUrl,
                             const QString &modelName, QObject *parent)
    : AIService(parent), m_apiKey(apiKey), m_baseUrl(baseUrl), m_modelName(modelName) {}

GeminiBackend::~GeminiBackend() {
    m_cancelled = true;
    // WARNING: Do not add waitForFinished() — blocks GUI thread up to 30s.
    // QPointer guards in the lambda handle safe destruction.
}

void GeminiBackend::translate(const QByteArray &pngImageData,
                               const QString &targetLanguage) {
    m_cancelled = false;

    QString apiKey = m_apiKey;
    QString baseUrl = m_baseUrl;
    QString modelName = m_modelName;
    QString lang = targetLanguage;
    QByteArray imageData = pngImageData;
    QPointer<GeminiBackend> self(this);

    m_future = QtConcurrent::run([self, apiKey, baseUrl, modelName, lang, imageData]() {
        try {
            QString base64Image = QString::fromLatin1(imageData.toBase64());

            QString prompt = QString(
                "OCR the text in this image and translate it to %1. "
                "Return a JSON array of text blocks with their positions. "
                "Each block should have the translated text and a bounding box "
                "with normalized coordinates (0.0 to 1.0 relative to image dimensions). "
                "Format: {\"blocks\":[{\"text\":\"translated text\","
                "\"x\":0.1,\"y\":0.2,\"w\":0.3,\"h\":0.05}]} "
                "x,y is top-left corner. Return ONLY valid JSON, no markdown fences. "
                "If no text is found, return {\"blocks\":[]}."
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

            // Normalize base URL: strip trailing slash and /v1beta to avoid duplication
            QString normalizedUrl = baseUrl;
            while (normalizedUrl.endsWith('/'))
                normalizedUrl.chop(1);
            if (normalizedUrl.endsWith("/v1beta"))
                normalizedUrl.chop(7);

            QString url = QString("%1/v1beta/models/%2:generateContent?key=%3")
                .arg(normalizedUrl, modelName, apiKey);

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
            std::string content = result["candidates"][0]["content"]["parts"][0]["text"];
            QString raw = QString::fromStdString(content).trimmed();

            // Strip markdown code fences if present
            if (raw.startsWith("```")) {
                int firstNewline = raw.indexOf('\n');
                int lastFence = raw.lastIndexOf("```");
                if (firstNewline >= 0 && lastFence > firstNewline)
                    raw = raw.mid(firstNewline + 1, lastFence - firstNewline - 1).trimmed();
            }

            json blocksJson = json::parse(raw.toStdString());
            QVector<TextBlock> blocks;
            for (auto &b : blocksJson["blocks"]) {
                TextBlock tb;
                tb.text = QString::fromStdString(b["text"].get<std::string>());
                tb.bbox = QRectF(b["x"].get<double>(), b["y"].get<double>(),
                                 b["w"].get<double>(), b["h"].get<double>());
                blocks.append(tb);
            }

            if (!self) return;
            QMetaObject::invokeMethod(self.data(), [self, blocks]() {
                if (self) emit self->translationReady(blocks);
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
