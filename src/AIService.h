#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>

class AIService : public QObject {
    Q_OBJECT
public:
    explicit AIService(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~AIService() = default;

    virtual QString name() const = 0;
    virtual void translate(const QByteArray &pngImageData,
                           const QString &targetLanguage) = 0;
    virtual void cancel() {}

signals:
    void translationReady(const QString &translatedText);
    void translationFailed(const QString &errorMessage);
};
