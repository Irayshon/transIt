#pragma once

#include <QObject>
#include <QByteArray>
#include <QRectF>
#include <QString>
#include <QVector>

struct TextBlock {
    QString text;
    QRectF bbox; // normalized 0.0-1.0 relative to image dimensions
};

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
    void translationReady(const QVector<TextBlock> &blocks);
    void translationFailed(const QString &errorMessage);
};
