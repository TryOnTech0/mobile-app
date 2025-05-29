#ifndef CAMERAFRAMEPROCESSOR_H
#define CAMERAFRAMEPROCESSOR_H

#include <QObject>
#include <QImage>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QTimer>
#include <QtWebSockets/QWebSocket>

class CameraFrameProcessor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)
    Q_PROPERTY(bool arActive READ isArActive NOTIFY arActiveChanged)
    Q_PROPERTY(bool cameraReady READ isCameraReady NOTIFY cameraReadyChanged)

public:
    explicit CameraFrameProcessor(QObject *parent = nullptr);
    ~CameraFrameProcessor();

    // Property getters
    bool isArActive() const;
    bool isCameraReady() const;
    QObject* videoSink() const;
    
    // Property setters
    void setVideoSink(QObject* videoSink);

    // AR session management
    Q_INVOKABLE void startArProcessing(const QString &serverUrl);
    Q_INVOKABLE void stopArProcessing();
    Q_INVOKABLE void monitorCameraStatus(QObject* camera);

signals:
    void arActiveChanged(bool active);
    void cameraReadyChanged(bool ready);
    void videoSinkChanged(QObject* videoSink);
    void arErrorOccurred(const QString &error);
    void processedFrameReady(const QImage &frame);
    void cameraStatusChanged(int status);

private slots:
    void handleVideoFrame(const QVideoFrame &frame);
    void handleWebSocketConnected();
    void handleWebSocketDisconnected();
    void handleBinaryMessageReceived(const QByteArray &message);
    void handleWebSocketError(QAbstractSocket::SocketError error);
    void checkCameraStatus();

private:
    void processArFrame(const QByteArray &frameData);
    void sendFrameForProcessing(const QVideoFrame &frame);
    QByteArray convertImageToBytes(const QImage &image);

    // Core components
    QWebSocket m_webSocket;
    QTimer m_connectionTimer;
    QObject *m_monitoredCamera;
    
    // Video sinks
    QVideoSink* m_videoSink;           // The actual display sink
    QObject* m_currentVideoSink;       // QObject wrapper for QML
    
    // State management
    bool m_arActive;
    bool m_cameraReady;
    QString m_arServerUrl;
    
    // Performance optimization
    int m_frameSkipCounter;
    const int m_maxFrameSkip = 2; // Process every 3rd frame
    
    // Response handling
    QByteArray m_pendingResponse;
    quint32 m_expectedResponseSize = 0;
    bool m_waitingForSize = true;
};

#endif // CAMERAFRAMEPROCESSOR_H