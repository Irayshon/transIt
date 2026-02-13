#pragma once

#include "AIService.h"

class GeminiBackend : public AIService {
    Q_OBJECT
public:
    explicit GeminiBackend(const QString &apiKey, QObject *parent = nullptr);

    QString name() const override { return "Gemini"; }
    void translate(const QByteArray &pngImageData,
                   const QString &targetLanguage) override;
    void cancel() override;

private:
    QString m_apiKey;
    bool m_cancelled = false;
};
