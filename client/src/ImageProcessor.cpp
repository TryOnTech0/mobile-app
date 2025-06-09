// ImageProcessor.cpp
#include "ImageProcessor.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QHttpPart>

ImageProcessor::ImageProcessor(QObject *parent) 
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

ImageProcessor::~ImageProcessor() {
    cleanupRequest();
}

QUrl ImageProcessor::serverUrl() const {
    return m_serverUrl;
}

void ImageProcessor::setServerUrl(const QUrl &url) {
    if (m_serverUrl == url) return;
    m_serverUrl = url;
    emit serverUrlChanged();
}

void ImageProcessor::cleanupRequest() {
    if (m_currentReply) {
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

void ImageProcessor::handleCapturedImage(const QByteArray &jpgData, const QString &garmentId) {
    cleanupRequest();

    if (m_serverUrl.isEmpty()) {
        emit processingError("Server URL not set");
        return;
    }

    emit processingProgress(0.1);

    // Create multipart form data
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Add garment ID as form field
    QHttpPart garmentIdPart;
    garmentIdPart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                           QVariant("form-data; name=\"garmentId\""));
    garmentIdPart.setBody(garmentId.toUtf8());
    multiPart->append(garmentIdPart);

    // Add image data as form field
    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                       QVariant("form-data; name=\"image\"; filename=\"capture.jpg\""));
    imagePart.setBody(jpgData);
    multiPart->append(imagePart);

    // Create request
    QNetworkRequest request(m_serverUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "ARClothTryOn/1.0");

    emit processingProgress(0.3);

    // Send POST request
    m_currentReply = m_networkManager->post(request, multiPart);
    multiPart->setParent(m_currentReply); // Delete multiPart with reply

    // Connect signals
    connect(m_currentReply, &QNetworkReply::finished, 
            this, &ImageProcessor::onReplyFinished);
    connect(m_currentReply, &QNetworkReply::uploadProgress, 
            this, &ImageProcessor::onUploadProgress);

    qDebug() << "Sending image to server:" << m_serverUrl.toString();
}

void ImageProcessor::onUploadProgress(qint64 bytesSent, qint64 bytesTotal) {
    if (bytesTotal > 0) {
        // Map upload progress to 30-60% of total progress
        qreal uploadProgress = static_cast<qreal>(bytesSent) / bytesTotal;
        emit processingProgress(0.3 + (uploadProgress * 0.3));
    }
}

void ImageProcessor::onReplyFinished() {
    if (!m_currentReply) return;

    emit processingProgress(0.8);

    QNetworkReply::NetworkError error = m_currentReply->error();
    
    if (error == QNetworkReply::NoError) {
        // Check HTTP status code
        int httpStatus = m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        
        if (httpStatus == 200) {
            // Success - get response data
            QByteArray responseData = m_currentReply->readAll();
            
            // Check content type to determine if it's an image or error message
            QString contentType = m_currentReply->header(QNetworkRequest::ContentTypeHeader).toString();
            
            if (contentType.startsWith("image/")) {
                emit processingProgress(1.0);
                emit processedImageReceived(responseData);
            } else {
                // Assume it's an error message
                QString errorMsg = QString::fromUtf8(responseData);
                emit processingError("Server error: " + errorMsg);
            }
        } else {
            // HTTP error
            QString errorMsg = QString("HTTP Error %1: %2")
                              .arg(httpStatus)
                              .arg(m_currentReply->readAll());
            emit processingError(errorMsg);
        }
    } else {
        // Network error
        emit processingError("Network error: " + m_currentReply->errorString());
    }

    cleanupRequest();
}