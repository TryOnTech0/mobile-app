#pragma once
#ifndef QMLMANAGER_H
#define QMLMANAGER_H

#include <QObject>
#include <QStringList>
#include <QImage>
#include <QPermission>
#include <QtQml/qqmlregistration.h>
#include <QGuiApplication>
#include <QDateTime>
#include <QVariantList>
#include <QVariantMap>
#include <QJsonArray>
#include <QJsonObject>
#include <memory>
#include "BodyTracker.h"
#include "ClothFitter.h"
#include "ClothScanner.h"
#include "NetworkManager.h"

class QMLManager : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int scanProgress READ scanProgress NOTIFY scanProgressChanged)
    Q_PROPERTY(QVariantList garments READ garments NOTIFY garmentsChanged)
    Q_PROPERTY(bool isNetworkConnected READ isNetworkConnected NOTIFY networkStatusChanged)

public:
    explicit QMLManager(QObject* parent = nullptr);
    ~QMLManager() = default;

    // Q_INVOKABLE methods for QML
    Q_INVOKABLE void uploadNewGarment();
    Q_INVOKABLE void initializeApp();
    Q_INVOKABLE void startScanning();
    Q_INVOKABLE void saveScan();
    Q_INVOKABLE void tryOnGarment(const QString& garmentId);
    Q_INVOKABLE void requestCameraPermission();
    Q_INVOKABLE bool hasCameraPermission() const;
    Q_INVOKABLE void fetchGarments(bool forceRefresh = false);
    Q_INVOKABLE void handleCapturedFrame(const QImage& frame, const QString& garmentId);
    // Q_INVOKABLE void fetchGarments();
    Q_INVOKABLE void setScanCategory(const QString& category);
    Q_INVOKABLE void saveGarment(const QString& garmentId,
                             const QString& name, 
                             const QString& previewUrl, 
                             const QString& modelUrl);
    // Property getters
    int scanProgress() const;
    QVariantList garments() const;
    bool isNetworkConnected() const { return m_networkConnected; }
    

signals:
    // Permission signals
    void permissionGranted();
    void permissionDenied();
    
    // Scanning signals
    void scanProgressChanged(int progress);
    void scanCompleted();
    void scanFailed(const QString& error);
    void scanProcessingFailed(const QString& error);  // ADDED MISSING SIGNAL
    
    // Garment signals
    void garmentsChanged();
    void garmentLoadFailed(const QString& error);
    void processedModelUrlReady(const QString& modelUrl, const QString& previewUrl);
    
    // AR signals
    void arSessionReady();
    void arSessionFailed(const QString& error);
    
    // Preview signals
    void showPreview(const QString& previewPath);
    
    // Network signals
    void networkStatusChanged(bool connected);
    void uploadProgressChanged(int progress);
    void uploadCompleted(const QString& garmentId);
    void uploadFailed(const QString& error);

public slots:
    // Permission handling
    void handlePermissionResult(const QPermission &permission);
    
    // Frame processing
    void handleFrame(const QImage& frame);
    
    // Progress updates
    void updateScanProgress(int progress);

private slots:
    // Network response handlers
    void handleGarmentsReceived(const QJsonArray& garments);
    void handleNetworkStatusChanged(bool connected);
    void handleUploadProgress(int progress);

private:
    // Core components
    std::unique_ptr<ClothScanner> m_clothScanner;
    std::unique_ptr<ClothFitter> m_clothFitter;
    std::unique_ptr<BodyTracker> m_bodyTracker;
    std::unique_ptr<NetworkManager> m_networkManager;
    
    // Data members
    QVariantList m_garments;
    int m_scanProgress = 0;
    bool m_networkConnected = false;
    QString m_currentCategory;
    
    // Helper methods
    void setupConnections();
    void resetScanState();
    QVariantMap createGarmentEntry(const QJsonObject& garmentObj);
};

#endif // QMLMANAGER_H