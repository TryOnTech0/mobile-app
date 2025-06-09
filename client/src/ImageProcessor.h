// ImageProcessor.h
#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>
#include <QUrl>

class ImageProcessor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)

public:
    explicit ImageProcessor(QObject *parent = nullptr);
    ~ImageProcessor();

    QUrl serverUrl() const;
    void setServerUrl(const QUrl &url);

    Q_INVOKABLE void handleCapturedImage(const QByteArray &jpgData, const QString &garmentId);

signals:
    void processedImageReceived(const QByteArray &imageData);
    void processingProgress(qreal progress);
    void processingError(const QString &errorMessage);
    void serverUrlChanged();

private slots:
    void onReplyFinished();
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    void cleanupRequest();

    QNetworkAccessManager *m_networkManager = nullptr;
    QNetworkReply *m_currentReply = nullptr;
    QUrl m_serverUrl;
};

#endif // IMAGEPROCESSOR_H

