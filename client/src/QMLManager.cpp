#include "QMLManager.h"
#include <QGuiApplication>
#include "ClothFitter.h"
#include "ImageConverter.h"
#include <QUrl>
#include <QLocale>
#include <QStandardPaths>  // Added missing include
#include <QDir>            // Added missing include
#include <QFileInfo>
#include <QBuffer>

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
    connect(m_networkManager.get(), &NetworkManager::scanUploaded,
            this, [this](const QString& garmentId, const QString& imageUrl) {
        qDebug() << "Scan uploaded. Image ID:" << garmentId;
        m_networkManager->getProcessedModel(garmentId);
    });

    connect(m_networkManager.get(), &NetworkManager::processedModelReady,
        this, [this](const QString& modelUrl, const QString& previewUrl, const QString& modelKey, const QString& previewKey) {
            emit processedModelUrlReady(modelUrl, previewUrl, modelKey, previewKey);
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

// Initialize the application
void QMLManager::initializeApp() {
    // Connect to database and fetch garments on app startup
    if (m_networkManager) {
        // uploadNewGarment();
        // m_networkManager->fetchGarments(true);
    }
}

void QMLManager::handleNetworkStatusChanged(bool isConnected) {
    qDebug() << "Network status changed. Connected:" << isConnected;
    // You can emit a QML signal or update UI state here if needed
}

void QMLManager::handleUploadProgress(int percent) {
    qDebug() << "Upload progress:" << percent << "%";
    // You can emit a signal to QML for progress updates here
}

void QMLManager::handleCapturedFrame(const QImage& frame, const QString& garmentId) {
    if(frame.isNull()) return;

    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    frame.save(&buffer, "JPEG", 85);
    
    if(m_networkManager) {
        if(m_currentCategory.isEmpty()) {
            qWarning() << "Category not selected";
            emit scanProcessingFailed("Please select a category");
        }
        m_networkManager->uploadScan(imageData, m_currentCategory, garmentId);
    }
}

// Add category setter
void QMLManager::setScanCategory(const QString& category) {
    m_currentCategory = category;
}

// Handle received garments from network
void QMLManager::handleGarmentsReceived(const QJsonArray& garments) {
    m_garments.clear();
    
    for (const QJsonValue& garmentValue : garments) {
        QJsonObject garmentObj = garmentValue.toObject();
        QVariantMap entry;
        
        // Corrected key: "id" instead of "garmentId"
        entry["id"] = garmentObj["garmentId"].toString(); // Match QML's model.modelData.id
        entry["name"] = garmentObj["name"].toString();
        entry["previewUrl"] = garmentObj["previewUrl"].toString();
        entry["modelUrl"] = garmentObj["modelUrl"].toString();
        entry["createdBy"] = garmentObj["createdBy"].toString();
        
        // Add "isAvailable" with a default value if missing in the data
        entry["isAvailable"] = true; // Ensure QML's model.modelData.isAvailable exists
        
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

// void QMLManager::scanProcessingFailed(const QString& error) {
//     qWarning() << "Scan processing failed:" << error;
//     emit scanProcessingFailed(error);
// }


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
       
    }
}

void QMLManager::saveGarment(const QString& garmentId,
                             const QString& name,
                             const QString& modelUrl,
                             const QString& previewUrl, 
                             const QString& modelKey,
                             const QString& previewKey,
                             const QString& category) {
    // Create garment data
    QJsonObject garmentData;
    garmentData["name"] = name;    
    garmentData["garmentId"] = garmentId; 
    garmentData["modelUrl"] = modelUrl;
    garmentData["previewUrl"] = previewUrl;
    garmentData["modelKey"] = modelKey;
    garmentData["previewKey"] = previewKey;
    garmentData["category"] = category;
    // Upload garment (preview will be generated server-side)
    m_networkManager->uploadGarment(garmentData);
}

// void QMLManager::uploadNewGarment() {
//     if (!m_networkManager) {
//         qWarning() << "NetworkManager not initialized";
//         return;
//     }

//     // Helper function to copy asset to temporary file
//     auto copyAssetToTemp = [](const QString &assetPath) -> QString {
//         QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
//         QDir().mkpath(tempDir);
        
//         QString fileName = QFileInfo(assetPath).fileName();
//         QString tempPath = tempDir + "/" + fileName;
        
//         // Remove existing temp file
//         QFile::remove(tempPath);
        
//         // Copy from assets
//         if (QFile::copy(assetPath, tempPath)) {
//             // Make sure the temp file is readable
//             QFile::setPermissions(tempPath, QFile::ReadOwner | QFile::WriteOwner);
//             qDebug() << "Copied asset to temp:" << tempPath;
//             return tempPath;
//         } else {
//             qWarning() << "Failed to copy asset:" << assetPath << "to" << tempPath;
//             return QString();
//         }
//     };

//     // Platform-independent path resolution
//     auto resolvePath = [&](const QString &relativePath) -> QString {
//         // Try different asset path formats for Android
//         QStringList assetPaths = {
//             "assets:/" + relativePath,
//             ":/" + relativePath,
//             "qrc:/" + relativePath,
//             ":/assets/" + relativePath
//         };
        
//         for (const QString &assetPath : assetPaths) {
//             if (QFile::exists(assetPath)) {
//                 qDebug() << "Found asset at:" << assetPath;
//                 // Copy to temporary location for upload
//                 return copyAssetToTemp(assetPath);
//             }
//         }
        
//         // Fallback for desktop development
//         QString desktopPath = QCoreApplication::applicationDirPath() + "/" + relativePath;
//         if (QFile::exists(desktopPath)) {
//             qDebug() << "Found desktop file at:" << desktopPath;
//             return desktopPath;
//         }
        
//         qWarning() << "Asset not found in any location:" << relativePath;
//         return QString();
//     };

//     // Upload shirt
//     QString shirtPreview = resolvePath("garments/shirt/preview.jpg");
//     QString shirtModel = resolvePath("garments/shirt/model.obj");
    
//     if (!shirtPreview.isEmpty() && !shirtModel.isEmpty()) {
//         QJsonObject shirtData;
//         shirtData["name"] = "White Shirt";
        
//         qDebug() << "Uploading shirt with preview:" << shirtPreview << "model:" << shirtModel;
//         m_networkManager->uploadGarment(shirtData, shirtPreview, shirtModel);
//     } else {
//         qWarning() << "Shirt files not found - Preview:" << shirtPreview << "Model:" << shirtModel;
//     }

//     // Upload pants
//     QString pantsPreview = resolvePath("garments/pants/preview.jpg");
//     QString pantsModel = resolvePath("garments/pants/model.obj");
    
//     if (!pantsPreview.isEmpty() && !pantsModel.isEmpty()) {
//         QJsonObject pantsData;
//         pantsData["name"] = "Classic Pants";
        
//         qDebug() << "Uploading pants with preview:" << pantsPreview << "model:" << pantsModel;
//         m_networkManager->uploadGarment(pantsData, pantsPreview, pantsModel);
//     } else {
//         qWarning() << "Pants files not found - Preview:" << pantsPreview << "Model:" << pantsModel;
//     }
// }


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
