#pragma once

#include "AIService.h"
#include <QFuture>
#include <QPointer>
#include <atomic>

class GeminiBackend : public AIService {
    Q_OBJECT
public:
    explicit GeminiBackend(const QString &apiKey, QObject *parent = nullptr);
    ~GeminiBackend() override;

    QString name() const override { return "Gemini"; }
    void translate(const QByteArray &pngImageData,
                   const QString &targetLanguage) override;
    void cancel() override;

private:
    QString m_apiKey;
    std::atomic_bool m_cancelled{false};
    QFuture<void> m_future;
};
