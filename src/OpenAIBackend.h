#pragma once

#include "AIService.h"
#include <QThread>

class OpenAIBackend : public AIService {
    Q_OBJECT
public:
    explicit OpenAIBackend(const QString &apiKey, QObject *parent = nullptr);

    QString name() const override { return "OpenAI"; }
    void translate(const QByteArray &pngImageData,
                   const QString &targetLanguage) override;
    void cancel() override;

private:
    QString m_apiKey;
    bool m_cancelled = false;
};
