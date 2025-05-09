#include "QMLManager.h"
#include <QGuiApplication>
#include "ClothFitter.h"
#include "ImageConverter.h"
#include <QUrl>

QMLManager::QMLManager(QObject* parent)
    : QObject(parent),
    m_clothScanner(std::make_unique<ClothScanner>()),
    m_clothFitter(std::make_unique<ClothFitter>()),
    m_bodyTracker(std::make_unique<BodyTracker>())
{}

// void QMLManager::requestCameraPermission() {
//     QCameraPermission cameraPermission;
//     const auto status = qApp->checkPermission(cameraPermission);
//     if (status == Qt::PermissionStatus::Granted) {
//         emit permissionGranted();
//     } else if (status == Qt::PermissionStatus::Denied) {
//         emit permissionDenied();
//     } else {
//         qApp->requestPermission(cameraPermission, this,
//                                 &QMLManager::handlePermissionResult);
//     }
// }

void QMLManager::requestCameraPermission() {
    QCameraPermission cameraPermission;
    qApp->requestPermission(cameraPermission, this, &QMLManager::handlePermissionResult);
}

void QMLManager::handlePermissionResult(const QPermission &permission) {
    if(permission.status() == Qt::PermissionStatus::Granted) {
        emit permissionGranted();
    } else {
        emit permissionDenied();
    }
}

bool QMLManager::hasCameraPermission() const {
    QCameraPermission cameraPermission;
    return qApp->checkPermission(cameraPermission) == Qt::PermissionStatus::Granted;
}
void QMLManager::startScanning() {
    // Implementation example
    if (!m_clothScanner) {
        m_clothScanner = std::make_unique<ClothScanner>();
    }
    m_clothScanner->captureFromCamera(0);  // Assuming this method exists
    emit scanProgressChanged(10);  // Example progress update
}

// void QMLManager::tryOnGarment(const QString& garmentId) {
//     m_clothFitter->loadClothModel(garmentId.toStdString());
//     m_bodyTracker->initCamera();
//     emit arSessionReady();
// }

void QMLManager::updateScanProgress(int percent) {
    if (m_scanProgress != percent) {
        m_scanProgress = percent;
        emit scanProgressChanged(percent);
    }
}

void QMLManager::saveScan() {
    // Implementation
}


int QMLManager::scanProgress() const {
    return m_scanProgress;
}

// QStringList QMLManager::garments() const {
//     return m_garments;
// }

void QMLManager::handleFrame(const QImage& frame) {
    // cv::Mat cvFrame = ImageConverter::qImageToCvMat(frame);
    // Process frame
}

QVariantList QMLManager::garments() const {
    return m_garments;
}

void QMLManager::loadGarments() {
    m_garments.clear();

    // Test Garment 1: Shirt
    QVariantMap shirt;
    shirt["id"] = "shirt_001";
    shirt["name"] = "Casual Shirt";
    shirt["previewUrl"] = QUrl("qrc:/garments/shirt/preview.jpg");
    shirt["isAvailable"] = true;
    m_garments.append(shirt);

    // Test Garment 2: Pants
    QVariantMap pants;
    pants["id"] = "pants_001";
    pants["name"] = "Formal Pants";
    pants["previewUrl"] = QUrl("qrc:/garments/pants/preview.jpg");
    pants["isAvailable"] = true;
    m_garments.append(pants);

    emit garmentsChanged();
}

// void QMLManager::loadGarments() {
//     // Implement database loading logic
//     m_garments = {"Shirt1", "Pants1", "Dress1"}; // Example data
//     emit garmentsChanged();
// }
// void QMLManager::loadGarments() {
//     // Example implementation
//     m_garments.clear();

//     // Replace with actual database access
//     QSqlQuery query("SELECT * FROM garments");
//     while (query.next()) {
//         QVariantMap garment;
//         garment["id"] = query.value("id");
//         garment["name"] = query.value("name");
//         garment["previewUrl"] = query.value("preview_path");
//         garment["isAvailable"] = query.value("available");
//         m_garments.append(garment);
//     }

//     emit garmentsChanged();
// }

void QMLManager::tryOnGarment(const QString& garmentId) {
    // Implement AR try-on logic
    m_bodyTracker->initCamera();
    m_clothFitter->loadClothModel(garmentId.toStdString());

    // Connect body tracking to cloth fitting
    connect(m_bodyTracker.get(), &BodyTracker::keypointsUpdated, [this]() {
        m_clothFitter->updateTransformation(
            m_bodyTracker->getKeypoints()
        );
    });

    emit arSessionReady();
}
