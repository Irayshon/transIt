#pragma once

#include "AIService.h"
#include <QFuture>
#include <QPointer>
#include <atomic>

class OpenAIBackend : public AIService {
    Q_OBJECT
public:
    explicit OpenAIBackend(const QString &apiKey, QObject *parent = nullptr);
    ~OpenAIBackend() override;

    QString name() const override { return "OpenAI"; }
    void translate(const QByteArray &pngImageData,
                   const QString &targetLanguage) override;
    void cancel() override;

private:
    QString m_apiKey;
    std::atomic_bool m_cancelled{false};
    QFuture<void> m_future;
};
