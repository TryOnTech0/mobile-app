#pragma once
#ifndef QMLMANAGER_H
#define QMLMANAGER_H

#include <QObject>
#include <QStringList>
#include <QImage>
#include <QPermission>
#include <QtQml/qqmlregistration.h>
#include <QGuiApplication>
// #include <QtQml>
#include <memory>
#include "BodyTracker.h"
#include "ClothFitter.h"
#include "ClothScanner.h"

class QMLManager : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int scanProgress READ scanProgress NOTIFY scanProgressChanged)
    // Q_PROPERTY(QStringList garments READ garments NOTIFY garmentsChanged)
    Q_PROPERTY(QVariantList garments READ garments NOTIFY garmentsChanged)
public:
    explicit QMLManager(QObject* parent = nullptr);


    Q_INVOKABLE void startScanning();
    Q_INVOKABLE void saveScan();
    Q_INVOKABLE void loadGarments();
    Q_INVOKABLE void tryOnGarment(const QString& garmentId);
    Q_INVOKABLE void requestCameraPermission();
    Q_INVOKABLE bool hasCameraPermission() const;

    int scanProgress() const;
    QVariantList garments() const;
    // QStringList garments() const;

signals:
    void permissionGranted();
    void permissionDenied();
    void scanProgressChanged(int progress);
    void garmentsChanged();
    void arSessionReady();
    void showPreview(const QString& previewPath);

private slots:
    Q_INVOKABLE void handlePermissionResult(const QPermission &permission);
    Q_INVOKABLE void handleFrame(const QImage& frame);
    Q_INVOKABLE void updateScanProgress(int progress);

private:
    std::unique_ptr<ClothScanner> m_clothScanner;
    std::unique_ptr<ClothFitter> m_clothFitter;
    std::unique_ptr<BodyTracker> m_bodyTracker;
    QVariantList m_garments;
    // QStringList m_garments;
    int m_scanProgress = 0;
};

#endif // QMLMANAGER_H
