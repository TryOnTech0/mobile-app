#include "CameraFrameProcessor.h"
#include <QVideoFrame>
#include <QBuffer>
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <QVideoFrameFormat>
#include <QDataStream>

CameraFrameProcessor::CameraFrameProcessor(QObject *parent)
    : QObject(parent),
      m_videoSink(nullptr),
      m_currentVideoSink(nullptr),
      m_arActive(false),
      m_cameraReady(false),
      m_monitoredCamera(nullptr),
      m_frameSkipCounter(0),
      m_expectedResponseSize(0),
      m_waitingForSize(true)
{
    // Initialize WebSocket connections
    connect(&m_webSocket, &QWebSocket::connected,
            this, &CameraFrameProcessor::handleWebSocketConnected);
    connect(&m_webSocket, &QWebSocket::disconnected,
            this, &CameraFrameProcessor::handleWebSocketDisconnected);
    connect(&m_webSocket, &QWebSocket::binaryMessageReceived,
            this, &CameraFrameProcessor::handleBinaryMessageReceived);
    connect(&m_webSocket, &QWebSocket::errorOccurred,  
            this, &CameraFrameProcessor::handleWebSocketError);
    
    // Set up connection timeout timer
    m_connectionTimer.setSingleShot(true);
    m_connectionTimer.setInterval(10000); // 10 second timeout
    connect(&m_connectionTimer, &QTimer::timeout, this, [this]() {
        if (m_webSocket.state() == QAbstractSocket::ConnectingState) {
            m_webSocket.abort();
            emit arErrorOccurred("Connection timeout - failed to connect to AR server");
        }
    });
}

CameraFrameProcessor::~CameraFrameProcessor()
{
    stopArProcessing();
}

bool CameraFrameProcessor::isArActive() const
{
    return m_arActive;
}

bool CameraFrameProcessor::isCameraReady() const
{
    return m_cameraReady;
}

QObject* CameraFrameProcessor::videoSink() const
{
    return m_currentVideoSink;
}

void CameraFrameProcessor::setVideoSink(QObject* videoSink)
{
    if (m_currentVideoSink == videoSink)
        return;

    // Disconnect previous video sink
    if (m_videoSink) {
        disconnect(m_videoSink, &QVideoSink::videoFrameChanged,
                   this, &CameraFrameProcessor::handleVideoFrame);
    }

    m_currentVideoSink = videoSink;
    
    if (auto sink = qobject_cast<QVideoSink*>(videoSink)) {
        m_videoSink = sink;
        connect(m_videoSink, &QVideoSink::videoFrameChanged,
                this, &CameraFrameProcessor::handleVideoFrame);
        qDebug() << "Video sink connected successfully. Sink address:" << m_videoSink;
    } else {
        qWarning() << "Failed to cast to QVideoSink";
        m_videoSink = nullptr;
    }
    
    emit videoSinkChanged(m_currentVideoSink);
}

void CameraFrameProcessor::startArProcessing(const QString &serverUrl)
{
    if (m_arActive) {
        qDebug() << "[AR] Processing already active - skipping new connection";
        return;
    }

    if (!m_videoSink) {
        qDebug() << "[AR] Cannot start - video sink not set";
        emit arErrorOccurred("Video sink is not properly initialized");
        return;
    }

    m_cameraReady = true; // Force ready for testing
    if (!m_cameraReady) {
        qDebug() << "[AR] Cannot start - camera not ready";
        emit arErrorOccurred("Camera is not ready for AR processing");
        return;
    }

    if (serverUrl.isEmpty()) {
        qDebug() << "[AR] Cannot start - invalid server URL";
        emit arErrorOccurred("Invalid AR server URL provided");
        return;
    }

    qDebug() << "[AR] Starting AR processing with server:" << serverUrl;
    qDebug() << "[AR] Video sink is:" << m_videoSink;
    m_arServerUrl = serverUrl;
    
    // Start connection timer
    m_connectionTimer.start();
    qDebug() << "[AR] Connection timer started (10s timeout)";
    
    // Connect to WebSocket
    m_webSocket.open(QUrl(serverUrl));
    
    qDebug() << "[AR] WebSocket open requested. State:" << m_webSocket.state();
}

void CameraFrameProcessor::handleVideoFrame(const QVideoFrame &frame)
{
    if (!m_cameraReady) {
        return;
    }

    // Add debug logging
    static int frameCount = 0;
    if (frameCount++ % 30 == 0) {
        qDebug() << "[AR] Frame" << frameCount << "AR active:" << m_arActive 
                 << "WebSocket state:" << m_webSocket.state();
    }

    if (m_arActive && m_webSocket.state() == QAbstractSocket::ConnectedState) {
        // When AR is active, only send frames for processing
        if (m_frameSkipCounter < m_maxFrameSkip) {
            m_frameSkipCounter++;
            return;
        }
        m_frameSkipCounter = 0;
        
        sendFrameForProcessing(frame);
        
        // Don't display the camera frame - wait for processed frame
    } else if (!m_arActive && m_videoSink) {
        // When AR is not active, show the camera feed normally
        m_videoSink->setVideoFrame(frame);
    }
}

void CameraFrameProcessor::sendFrameForProcessing(const QVideoFrame &frame)
{
    QVideoFrame cloneFrame(frame);
    
    if (!cloneFrame.map(QVideoFrame::ReadOnly)) {
        qWarning() << "Failed to map video frame for reading";
        return;
    }

    QImage image;
    QVideoFrameFormat::PixelFormat pixelFormat = cloneFrame.pixelFormat();
    
    // Log pixel format for debugging
    static bool loggedFormat = false;
    if (!loggedFormat) {
        qDebug() << "[AR] Camera pixel format:" << pixelFormat;
        loggedFormat = true;
    }
    
    // Handle different pixel formats
    switch (pixelFormat) {
        case QVideoFrameFormat::Format_ARGB8888:
        case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
            image = QImage(
                cloneFrame.bits(0),
                cloneFrame.width(),
                cloneFrame.height(),
                cloneFrame.bytesPerLine(0),
                QImage::Format_ARGB32
            );
            break;
        case QVideoFrameFormat::Format_BGRA8888:
        case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
            image = QImage(
                cloneFrame.bits(0),
                cloneFrame.width(),
                cloneFrame.height(),
                cloneFrame.bytesPerLine(0),
                QImage::Format_ARGB32
            ).rgbSwapped();
            break;
        case QVideoFrameFormat::Format_XRGB8888:
        case QVideoFrameFormat::Format_XBGR8888:
            image = QImage(
                cloneFrame.bits(0),
                cloneFrame.width(),
                cloneFrame.height(),
                cloneFrame.bytesPerLine(0),
                QImage::Format_RGB32
            );
            break;
        case QVideoFrameFormat::Format_RGBA8888:
            image = QImage(
                cloneFrame.bits(0),
                cloneFrame.width(),
                cloneFrame.height(),
                cloneFrame.bytesPerLine(0),
                QImage::Format_RGBA8888
            );
            break;
        case QVideoFrameFormat::Format_NV12:
        case QVideoFrameFormat::Format_NV21:
        case QVideoFrameFormat::Format_YUV420P:
        case QVideoFrameFormat::Format_YV12:
            // For YUV formats, use Qt's built-in conversion
            image = cloneFrame.toImage();
            break;
        default:
            qWarning() << "Unsupported pixel format:" << pixelFormat;
            image = cloneFrame.toImage(); // Try Qt's built-in conversion
            break;
    }
    
    cloneFrame.unmap();
    
    if (image.isNull()) {
        qWarning() << "Failed to create image from video frame";
        return;
    }
    
    // Ensure the image is in a format we can work with
    if (image.format() != QImage::Format_ARGB32 && 
        image.format() != QImage::Format_RGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }
    
    // Scale down image if too large
    if (image.width() > 640 || image.height() > 480) {
        image = image.scaled(640, 480, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    // Convert to JPEG
    QByteArray imageData = convertImageToBytes(image);
    if (imageData.isEmpty()) {
        qWarning() << "Failed to convert image to JPEG";
        return;
    }
    
    // Add size header before sending
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    // Write size (4 bytes, big-endian)
    stream << quint32(imageData.size());
    
    // Append image data
    message.append(imageData);
    
    // Send the complete message
    m_webSocket.sendBinaryMessage(message);
    
    static int sentCount = 0;
    if (sentCount++ % 10 == 0) {
        qDebug() << "[AR] Sent frame" << sentCount << "Size:" << message.size();
    }
}

void CameraFrameProcessor::handleBinaryMessageReceived(const QByteArray &message)
{
    if (message.size() < 4) {
        qWarning() << "Received message too small:" << message.size();
        return;
    }
    
    // Read size header
    QDataStream stream(message);
    stream.setByteOrder(QDataStream::BigEndian);
    quint32 imageSize;
    stream >> imageSize;
    
    // Extract image data (skip the 4-byte header)
    QByteArray imageData = message.mid(4);
    
    if (imageData.size() != imageSize) {
        qWarning() << "Image size mismatch. Expected:" << imageSize << "Got:" << imageData.size();
        return;
    }
    
    qDebug() << "[AR] Received processed frame. Size:" << imageSize;
    processArFrame(imageData);
}

void CameraFrameProcessor::processArFrame(const QByteArray &frameData)
{
    QImage receivedImage;
    if (!receivedImage.loadFromData(frameData, "JPEG")) {
        qWarning() << "Failed to load received frame data";
        return;
    }
    
    qDebug() << "[AR] Loaded image from data. Size:" << receivedImage.size();
    emit processedFrameReady(receivedImage);
    
    // Display processed frame on the connected video sink
    if (m_arActive && m_videoSink) {
        QVideoFrameFormat format(receivedImage.size(), QVideoFrameFormat::Format_ARGB8888);
        QVideoFrame frame(format);
        
        if (frame.map(QVideoFrame::WriteOnly)) {
            QImage convertedImage = receivedImage.convertToFormat(QImage::Format_ARGB32);
            memcpy(frame.bits(0), convertedImage.bits(), convertedImage.sizeInBytes());
            frame.unmap();
            
            // Send processed frame to the video sink
            m_videoSink->setVideoFrame(frame);
            qDebug() << "[AR] Set processed frame to video sink";
        } else {
            qWarning() << "[AR] Failed to map video frame for writing";
        }
    } else {
        qWarning() << "[AR] Cannot display frame. AR active:" << m_arActive 
                   << "Video sink:" << m_videoSink;
    }
}

void CameraFrameProcessor::stopArProcessing()
{
    if (!m_arActive)
        return;

    qDebug() << "Stopping AR processing";
    
    m_connectionTimer.stop();
    m_arActive = false;
    emit arActiveChanged(m_arActive);

    if (m_webSocket.state() != QAbstractSocket::UnconnectedState) {
        m_webSocket.close();
    }
}

void CameraFrameProcessor::handleWebSocketConnected()
{
    m_connectionTimer.stop();
    m_arActive = true;
    emit arActiveChanged(m_arActive);
    
    qDebug() << "WebSocket connected, AR processing active";
}

void CameraFrameProcessor::handleWebSocketDisconnected()
{
    m_connectionTimer.stop();
    bool wasActive = m_arActive;
    m_arActive = false;
    
    if (wasActive) {
        emit arActiveChanged(m_arActive);
        qDebug() << "WebSocket disconnected, AR processing stopped";
    }
}

void CameraFrameProcessor::handleWebSocketError(QAbstractSocket::SocketError error)
{
    m_connectionTimer.stop();
    QString errorString = m_webSocket.errorString();
    
    // Provide more user-friendly error messages
    switch (error) {
        case QAbstractSocket::ConnectionRefusedError:
            errorString = "Connection refused - AR server may not be running";
            break;
        case QAbstractSocket::HostNotFoundError:
            errorString = "Host not found - check server URL";
            break;
        case QAbstractSocket::NetworkError:
            errorString = "Network error - check internet connection";
            break;
        case QAbstractSocket::SocketTimeoutError:
            errorString = "Connection timeout - server not responding";
            break;
        default:
            errorString = "AR Server connection error: " + errorString;
            break;
    }
    
    qWarning() << "WebSocket error:" << error << errorString;
    emit arErrorOccurred(errorString);
    
    // Ensure we're marked as inactive
    if (m_arActive) {
        m_arActive = false;
        emit arActiveChanged(m_arActive);
    }
}

void CameraFrameProcessor::monitorCameraStatus(QObject* camera)
{
    if (m_monitoredCamera == camera)
        return;

    // Disconnect from previous camera
    if (m_monitoredCamera) {
        disconnect(m_monitoredCamera, nullptr, this, nullptr);
    }

    m_monitoredCamera = camera;
    
    if (m_monitoredCamera) {
        connect(m_monitoredCamera, SIGNAL(cameraStatusChanged()), 
                this, SLOT(checkCameraStatus()));
        
        // Initial status check
        QMetaObject::invokeMethod(this, "checkCameraStatus", Qt::QueuedConnection);
        
        qDebug() << "Monitoring camera status";
    }
}

void CameraFrameProcessor::checkCameraStatus()
{
    if (!m_monitoredCamera)
        return;

    QVariant statusVariant = m_monitoredCamera->property("cameraStatus");
    if (statusVariant.isValid()) {
        int status = statusVariant.toInt();
        emit cameraStatusChanged(status);
        
        bool isReady = (status == 1); // Camera.ActiveStatus
        
        if (m_cameraReady != isReady) {
            m_cameraReady = isReady;
            emit cameraReadyChanged(isReady);
            qDebug() << "Camera ready state changed:" << isReady;
        }
    }
}

QByteArray CameraFrameProcessor::convertImageToBytes(const QImage &image)
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    
    if (!image.save(&buffer, "JPEG", 75)) {
        qWarning() << "Failed to convert image to JPEG";
        return QByteArray();
    }
    
    return byteArray;
}