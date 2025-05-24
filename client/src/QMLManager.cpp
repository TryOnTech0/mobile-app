#include "QMLManager.h"
#include <QGuiApplication>
#include "ClothFitter.h"
#include "ImageConverter.h"
#include <QUrl>
#include <QLocale>

QMLManager::QMLManager(QObject* parent)
    : QObject(parent),
    m_clothScanner(std::make_unique<ClothScanner>()),
    m_clothFitter(std::make_unique<ClothFitter>()),
    m_bodyTracker(std::make_unique<BodyTracker>()),
    m_networkManager(std::make_unique<NetworkManager>()) // Initialize NetworkManager
{
    // Connect NetworkManager signals to QMLManager slots - Fixed connections
    connect(m_networkManager.get(), &NetworkManager::garmentsReceived,
            this, &QMLManager::handleGarmentsReceived);
    connect(m_networkManager.get(), &NetworkManager::connectionStatusChanged,
            this, &QMLManager::networkStatusChanged);
    
    // Additional useful connections for error handling
    connect(m_networkManager.get(), &NetworkManager::networkError,
            this, [this](const QString& error) {
                qWarning() << "Network error:" << error;
                // You can emit a signal to QML if needed
            });
    
    connect(m_networkManager.get(), &NetworkManager::garmentUploadSucceeded,
            this, [this](const QString& garmentId) {
                qDebug() << "Upload succeeded for garment:" << garmentId;
                // Refresh garments after successful upload
                m_networkManager->fetchGarments(true);
            });
    
    connect(m_networkManager.get(), &NetworkManager::garmentUploadFailed,
            this, [this](const QString& error) {
                qWarning() << "Upload failed:" << error;
                // Handle upload failure
            });
}
void QMLManager::handleNetworkStatusChanged(bool isConnected) {
    qDebug() << "Network status changed. Connected:" << isConnected;
    // You can emit a QML signal or update UI state here if needed
}

void QMLManager::handleUploadProgress(int percent) {
    qDebug() << "Upload progress:" << percent << "%";
    // You can emit a signal to QML for progress updates here
}

// Initialize the application
void QMLManager::initializeApp() {
    // Connect to database and fetch garments on app startup
    if (m_networkManager) {
        m_networkManager->connectToDatabase("tryonDB", "user", "password");
        uploadNewGarment();
        m_networkManager->fetchGarments();
    }
}

// Handle received garments from network
void QMLManager::handleGarmentsReceived(const QJsonArray& garments) {
    m_garments.clear();
    
    for (const QJsonValue& garmentValue : garments) {
        QJsonObject garmentObj = garmentValue.toObject();
        QVariantMap entry;
        
        // Required fields
        entry["id"] = garmentObj["_id"].toString();
        entry["name"] = garmentObj["name"].toString();
        entry["previewUrl"] = garmentObj["previewUrl"].toString();
        entry["modelUrl"] = garmentObj["modelUrl"].toString();
        
        // Category with validation
        QString category = garmentObj["category"].toString("shirt");
        entry["category"] = category;

        // Optional description
        if (garmentObj.contains("description")) {
            entry["description"] = garmentObj["description"].toString();
        }

        // User reference
        QJsonObject createdBy = garmentObj["createdBy"].toObject();
        if (!createdBy.isEmpty()) {
            entry["createdById"] = createdBy["_id"].toString();
            entry["createdByName"] = createdBy.value("username").toString();
        }

        // Date handling - Fixed potential crash
        QString dateString = garmentObj["createdAt"].toString();
        if (!dateString.isEmpty()) {
            QDateTime createdAt = QDateTime::fromString(dateString, Qt::ISODate);
            if (createdAt.isValid()) {
                entry["createdAt"] = createdAt.toString(QLocale::system().toString(createdAt.date(), QLocale::ShortFormat));
            } else {
                entry["createdAt"] = dateString; // Fallback to original string
            }
        }

        m_garments.append(entry);
    }
    
    emit garmentsChanged();
}

// Request camera permission
void QMLManager::requestCameraPermission() {
    QCameraPermission cameraPermission;
    qApp->requestPermission(cameraPermission, this, &QMLManager::handlePermissionResult);
}

// Handle permission result
void QMLManager::handlePermissionResult(const QPermission &permission) {
    if(permission.status() == Qt::PermissionStatus::Granted) {
        emit permissionGranted();
    } else {
        emit permissionDenied();
    }
}

// Check if camera permission is granted
bool QMLManager::hasCameraPermission() const {
    QCameraPermission cameraPermission;
    return qApp->checkPermission(cameraPermission) == Qt::PermissionStatus::Granted;
}

// Start cloth scanning process
void QMLManager::startScanning() {
    // Implementation example
    if (!m_clothScanner) {
        m_clothScanner = std::make_unique<ClothScanner>();
    }
    
    // Check camera permission before starting
    if (!hasCameraPermission()) {
        requestCameraPermission();
        return;
    }
    
    try {
        m_clothScanner->captureFromCamera(0);  // Assuming this method exists
        emit scanProgressChanged(10);  // Example progress update
    } catch (const std::exception& e) {
        qWarning() << "Failed to start scanning:" << e.what();
    }
}

// Update scan progress
void QMLManager::updateScanProgress(int percent) {
    if (m_scanProgress != percent) {
        m_scanProgress = percent;
        emit scanProgressChanged(percent);
    }
}

// Save current scan
void QMLManager::saveScan() {
    // Implementation for saving scan results
    if (m_clothScanner) {
        // Save scan data
        qDebug() << "Saving scan data...";
        // You can add actual implementation here
    }
}

// Get current scan progress
int QMLManager::scanProgress() const {
    return m_scanProgress;
}

// Handle camera frame processing
void QMLManager::handleFrame(const QImage& frame) {
    // Process the camera frame
    if (frame.isNull()) {
        return;
    }
    
    try {
        // cv::Mat cvFrame = ImageConverter::qImageToCvMat(frame);
        // Process frame with computer vision
        // This is where you'd implement your image processing logic
    } catch (const std::exception& e) {
        qWarning() << "Frame processing error:" << e.what();
    }
}

// Get garments list
QVariantList QMLManager::garments() const {
    return m_garments;
}

// Load test garments (fallback method)
// void QMLManager::loadGarments() {
//     m_garments.clear();

//     // Test Garment 1: Shirt
//     QVariantMap shirt;
//     shirt["id"] = "shirt_001";
//     shirt["name"] = "Casual Shirt";
//     shirt["previewUrl"] = QUrl("qrc:/garments/shirt/preview.jpg");
//     shirt["isAvailable"] = true;
//     shirt["category"] = "shirt";
//     m_garments.append(shirt);

//     // Test Garment 2: Pants
//     QVariantMap pants;
//     pants["id"] = "pants_001";
//     pants["name"] = "Formal Pants";
//     pants["previewUrl"] = QUrl("qrc:/garments/pants/preview.jpg");
//     pants["isAvailable"] = true;
//     pants["category"] = "pants";
//     m_garments.append(pants);

//     // Test Garment 3: Dress
//     QVariantMap dress;
//     dress["id"] = "dress_001";
//     dress["name"] = "Summer Dress";
//     dress["previewUrl"] = QUrl("qrc:/garments/dress/preview.jpg");
//     dress["isAvailable"] = true;
//     dress["category"] = "dress";
//     m_garments.append(dress);

//     emit garmentsChanged();
// }

// Fetch garments from network - Fixed to properly delegate to NetworkManager
void QMLManager::fetchGarments(bool forceRefresh) {
    if (m_networkManager) {
        m_networkManager->fetchGarments(forceRefresh);
    } else {
        qWarning() << "NetworkManager not initialized";
        // Fallback to loading test garments
        loadGarments();
    }
}

// Upload new garment with proper error handling
void QMLManager::uploadNewGarment() {
    if (!m_networkManager) {
        qWarning() << "NetworkManager not initialized";
        return;
    }

    // Example shirt data
    QJsonObject shirt_data;
    shirt_data["name"] = "White Shirt";
    shirt_data["category"] = "shirt";
    shirt_data["size"] = "M";
    shirt_data["description"] = "Classic white dress shirt";
    
    m_networkManager->uploadGarment(shirt_data,
                                  "../../server/uploads/previews/shirt.png",
                                  "../../server/uploads/models/shirt.obj");

    // Example pants data
    QJsonObject pants_data;
    pants_data["name"] = "Classic Pants";
    pants_data["category"] = "pants";
    pants_data["size"] = "M";
    pants_data["description"] = "Formal business pants";
    
    m_networkManager->uploadGarment(pants_data,
                                  "../../server/uploads/previews/pants.png",
                                  "../../server/uploads/models/pants.obj");
}

// Try on garment with AR - Fixed connection handling
void QMLManager::tryOnGarment(const QString& garmentId) {
    if (!m_bodyTracker || !m_clothFitter) {
        qWarning() << "Body tracker or cloth fitter not initialized";
        return;
    }

    try {
        // Initialize body tracking
        m_bodyTracker->initCamera();
        
        // Load cloth model
        m_clothFitter->loadClothModel(garmentId.toStdString());

        // Connect body tracking to cloth fitting - Fixed lambda connection
        // Disconnect any existing connections first
        disconnect(m_bodyTracker.get(), &BodyTracker::keypointsUpdated, nullptr, nullptr);
        
        // Create new connection with proper error handling
        connect(m_bodyTracker.get(), &BodyTracker::keypointsUpdated, this, [this]() {
            try {
                if (m_clothFitter && m_bodyTracker) {
                    m_clothFitter->updateTransformation(
                        m_bodyTracker->getKeypoints()
                    );
                }
            } catch (const std::exception& e) {
                qWarning() << "Error updating cloth transformation:" << e.what();
            }
        });

        emit arSessionReady();
        
    } catch (const std::exception& e) {
        qWarning() << "Failed to initialize AR try-on:" << e.what();
    }
}