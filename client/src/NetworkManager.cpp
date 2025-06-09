#include "NetworkManager.h"
#include <QAuthenticator>
#include <QDebug>
#include <QFile>
#include <QHttpMultiPart>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSslSocket>
#include <QSslConfiguration>
#include <memory>
#include <QThread>

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this))
{
    #ifdef Q_OS_ANDROID
        //eduroam
        // m_serverUrl = "http://10.1.239.113:5000/api";
        m_serverUrl = "http://192.168.1.23:5000/api";
        // Ata
        // m_serverUrl = "http://172.20.10.6:5000/api";
    #else
        m_serverUrl = "http://localhost:5000/api";
    #endif

    connect(m_networkManager, &QNetworkAccessManager::authenticationRequired,
            this, &NetworkManager::onAuthenticationRequired);

#ifndef QT_NO_SSL
    m_sslConfig = QSslConfiguration::defaultConfiguration();
    m_sslConfig.setProtocol(QSsl::TlsV1_2OrLater);

    connect(m_networkManager, &QNetworkAccessManager::sslErrors,
            this, &NetworkManager::onSslErrors);

    qDebug() << "SSL support available:" << QSslSocket::supportsSsl();
#else
    qDebug() << "SSL disabled - HTTP-only networking";
#endif

    m_authToken = loadAuthToken();
    verifyAuthToken();
    
}

// Destructor
NetworkManager::~NetworkManager() {

}
void NetworkManager::verifyServerConnectivity() {
    QNetworkRequest request(QUrl(m_serverUrl + "/api/status"));
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        bool ok;
        QJsonDocument response = parseJsonReply(reply, ok);
        if (ok && response.object().value("online").toBool()) {
            emit connectionStatusChanged(true);
        } else {
            emit connectionStatusChanged(false);
        }
        reply->deleteLater();
    });
}
void NetworkManager::verifyAuthToken() {
    if (m_authToken.isEmpty()) return;

    QNetworkRequest request = createAuthenticatedRequest(QUrl(m_serverUrl + "/auth/verify"));
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        bool ok;
        QJsonDocument response = parseJsonReply(reply, ok);
        if (ok && response.object()["valid"].toBool()) {
            emit connectionStatusChanged(true);
        } else {
            clearAuthToken();
            emit connectionStatusChanged(false);
        }
        reply->deleteLater();
    });
}

QNetworkRequest NetworkManager::createAuthenticatedRequest(const QUrl& url) {
    QNetworkRequest request(url);

#ifndef QT_NO_SSL
    if (url.scheme() == "https") {
        request.setSslConfiguration(m_sslConfig);
    }
#endif

    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    return request;
}

#ifndef QT_NO_SSL
void NetworkManager::onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors) {
    QString errorString;
    for (const QSslError& error : errors) {
        errorString += error.errorString() + ", ";
    }
    errorString.chop(2);

    qWarning() << "SSL errors:" << errorString;
    emit networkError("SSL error: " + errorString);
}
#endif

// Method for scan uploads

// Get server URL
QString NetworkManager::serverUrl() const {
    return m_serverUrl;
}

// Set server URL
void NetworkManager::setServerUrl(const QString& url) {
    if (m_serverUrl != url) {
        m_serverUrl = url;
        emit serverUrlChanged();
    }
}

// Fetch all garments
// ---- Fetch All Garments (GET /garments) ----
void NetworkManager::fetchGarments(bool forceRefresh) {
    QUrl url(m_serverUrl + "/garments");
    qDebug() << "Starting garments fetch from:" << url.toString();
    
    QNetworkRequest request = createAuthenticatedRequest(url);
    QNetworkReply *reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleGarmentsResponse(reply);
    });
    
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply](QNetworkReply::NetworkError error) {
        qDebug() << "Network error occurred during fetch";
        processNetworkError(error);
    });
}

void NetworkManager::handleGarmentsResponse(QNetworkReply* reply) {
    bool ok;
    QJsonDocument doc = parseJsonReply(reply, ok);
    
    if(ok && doc.isArray()) {
        QJsonArray garmentsArray = doc.array();
        QString jsonString = QString::fromUtf8(doc.toJson());
        qDebug().noquote() << "Fetched garments data:\n" << jsonString;
        emit garmentsReceived(garmentsArray);
    } else {
        qDebug() << "Failed to parse garments response. Raw response:"
                 << QString::fromUtf8(reply->readAll());
        emit networkError(tr("Failed to fetch garments."));
    }
    
    reply->deleteLater();
}

void NetworkManager::uploadScan(const QByteArray& imageData, const QString& category, const QString& garmentId) {
    qDebug() << "Uploading scan with:";
    qDebug() << " - Category:" << category;
    qDebug() << " - Garment ID:" << garmentId;
    qDebug() << " - Image size:" << imageData.size() << "bytes";

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    // Add garment ID part
    QHttpPart garmentIdPart;
    garmentIdPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant("form-data; name=\"garmentId\""));
    garmentIdPart.setBody(garmentId.toUtf8());  
    multiPart->append(garmentIdPart);
    
    // Add category part
    QHttpPart categoryPart;
    categoryPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                          QVariant("form-data; name=\"category\""));
    categoryPart.setBody(category.toUtf8());
    multiPart->append(categoryPart);

    // Add image part
    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"image\"; filename=\"scan.jpg\""));
    imagePart.setBody(imageData);
    multiPart->append(imagePart);

    QUrl url(m_serverUrl + "/scans");
    QNetworkRequest request = createAuthenticatedRequest(url);
    
    QNetworkReply* reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::uploadProgress, [this](qint64 sent, qint64 total) {
        if (total > 0) {
            emit scanProgressChanged(static_cast<int>((sent * 100) / total));
        }
    });

    connect(reply, &QNetworkReply::finished, [this, reply, garmentId]() {
        bool ok;
        QJsonDocument response = parseJsonReply(reply, ok);
        
        if (ok && response.isObject()) {
            QJsonObject responseObj = response.object();
            
            if ((responseObj.contains("success") && responseObj["success"].toBool()) ||
                responseObj.contains("garmentId") || 
                responseObj.contains("imageUrl")) {
                
                QString returnedGarmentId = responseObj.contains("garmentId") ? 
                                        responseObj["garmentId"].toString() : garmentId;
                
                QString imageUrl = responseObj.contains("imageUrl") ? 
                                responseObj["imageUrl"].toString() : "";
                
                qDebug() << "Scan upload successful for garment:" << returnedGarmentId;
                emit scanUploaded(returnedGarmentId, imageUrl);
            } else {
                QString errorMsg = responseObj.contains("error") ? 
                                 responseObj["error"].toString() : "Unknown upload error";
                qWarning() << "Scan upload failed:" << errorMsg;
                emit networkError("Scan upload failed: " + errorMsg);
            }
        } else {
            qWarning() << "Invalid response from scan upload";
            emit networkError("Scan upload failed: Invalid server response");
        }
        
        reply->deleteLater();
    });
}

void NetworkManager::getProcessedModel(const QString& garmentId) {
    qDebug() << "Requesting processed model for garment:" << garmentId;
    
    const int maxRetries = 10;
    const int retryDelayMs = 2000;

    // Build URL with garmentId as query parameter
    QUrl url(m_serverUrl + "/3d-models");
    QUrlQuery query;
    query.addQueryItem("garmentId", garmentId);
    url.setQuery(query);
    
    QNetworkRequest request = createAuthenticatedRequest(url);

    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        qDebug() << "Requesting processed model for garment" << garmentId 
                 << "(attempt" << (attempt + 1) << "of" << maxRetries << ")";

        QNetworkReply* reply = m_networkManager->get(request);

        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();  // BLOCKS until the reply is finished

        bool ok;
        QJsonDocument response = parseJsonReply(reply, ok);

        if (ok && response.isObject()) {
            QJsonObject obj = response.object();
            
            // Check if processing is complete
            if (obj.contains("status")) {
                QString status = obj["status"].toString();
                qDebug() << "Processing status for garment" << garmentId << ":" << status;
                
                if (status == "completed" && obj.contains("modelUrl")) {
                    QString modelUrl = obj["modelUrl"].toString();
                    QString previewUrl = obj["previewUrl"].toString();
                    QString modelKey = obj["modelKey"].toString();
                    QString previewKey = obj["previewKey"].toString();
                    qDebug() << "3D model ready for garment" << garmentId << ":" << modelUrl;
                    emit processedModelReady(modelUrl, previewUrl, modelKey, previewKey);
                    reply->deleteLater();
                    return;
                } else if (status == "failed") {
                    QString errorMsg = obj.contains("error") ? 
                                     obj["error"].toString() : "Processing failed";
                    qWarning() << "3D model processing failed for garment" << garmentId << ":" << errorMsg;
                    emit networkError("3D model processing failed: " + errorMsg);
                    reply->deleteLater();
                    return;
                } else if (status == "processing") {
                    qDebug() << "Model still processing for garment" << garmentId << "...";
                    // Continue to retry
                }
            } else if (obj.contains("modelUrl")) {
                // Fallback: direct modelUrl without status field
                QString modelUrl = obj["modelUrl"].toString();
                QString previewUrl = obj["previewUrl"].toString();
                QString modelKey = obj["modelKey"].toString();
                QString previewKey = obj["previewKey"].toString();
                qDebug() << "3D model ready for garment" << garmentId << ":" << modelUrl;
                emit processedModelReady(modelUrl, previewUrl, modelKey, previewKey);
                reply->deleteLater();
                return;
            }
        } else {
            qWarning() << "Invalid response when requesting model for garment" << garmentId;
        }

        reply->deleteLater();
        
        // Don't sleep on the last attempt
        if (attempt < maxRetries - 1) {
            qDebug() << "Model not ready yet for garment" << garmentId 
                     << ". Retrying in" << retryDelayMs << "ms...";
            QThread::msleep(retryDelayMs);
        }
    }

    qWarning() << "Failed to get 3D model for garment" << garmentId << ": Max retries reached";
    emit networkError("Failed to get 3D model for garment " + garmentId + ": Max retries reached");
}


// Upload a new garment
void NetworkManager::uploadGarment(const QJsonObject& garmentData) {
    
    qDebug() << "Starting garment upload";
    // qDebug() << "Model path:" << modelPath;
    qDebug() << "Garment data:" << garmentData;
    
    // // Validate files exist
    // if (!QFile::exists(previewPath)) {
    //     emit garmentUploadFailed(tr("Preview file not found: %1").arg(previewPath));
    //     return;
    // }
    
    // if (!QFile::exists(modelPath)) {
    //     emit garmentUploadFailed(tr("Model file not found: %1").arg(modelPath));
    //     return;
    // }
    
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    // Add garment metadata
    for(auto it = garmentData.begin(); it != garmentData.end(); ++it) {
        QHttpPart dataPart;
        dataPart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                          QVariant(QString("form-data; name=\"%1\"").arg(it.key())));
        dataPart.setBody(it.value().toString().toUtf8());
        multiPart->append(dataPart);
    }

    // Add preview file
    // QFile *previewFile = new QFile(previewPath);
    // if (!previewFile->open(QIODevice::ReadOnly)) {
    //     emit garmentUploadFailed(tr("Cannot open preview file: %1").arg(previewPath));
    //     delete previewFile;
    //     delete multiPart;
    //     return;
    // }
    
    // QHttpPart previewPart;
    // QString previewMimeType = QMimeDatabase().mimeTypeForFile(previewPath).name();
    // previewPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(previewMimeType));
    // previewPart.setHeader(QNetworkRequest::ContentDispositionHeader, 
    //     QVariant(QString("form-data; name=\"preview\"; filename=\"%1\"").arg(QFileInfo(previewPath).fileName())));
    // previewPart.setBodyDevice(previewFile);
    // previewFile->setParent(multiPart); // Ensure cleanup
    // multiPart->append(previewPart);

    // // Add model file
    // QFile *modelFile = new QFile(modelPath);
    // if (!modelFile->open(QIODevice::ReadOnly)) {
    //     emit garmentUploadFailed(tr("Cannot open model file: %1").arg(modelPath));
    //     delete modelFile;
    //     delete multiPart;
    //     return;
    // }
    
    // QHttpPart modelPart;
    // QString modelMimeType = QMimeDatabase().mimeTypeForFile(modelPath).name();
    // if (modelMimeType == "application/octet-stream" || modelMimeType.isEmpty()) {
    //     // Set specific MIME type for .obj files
    //     if (modelPath.endsWith(".obj", Qt::CaseInsensitive)) {
    //         modelMimeType = "text/plain"; // or "model/obj" if server supports it
    //     }
    // }
    
    // modelPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(modelMimeType));
    // modelPart.setHeader(QNetworkRequest::ContentDispositionHeader, 
    //     QVariant(QString("form-data; name=\"model\"; filename=\"%1\"").arg(QFileInfo(modelPath).fileName())));
    // modelPart.setBodyDevice(modelFile);
    // modelFile->setParent(multiPart); // Ensure cleanup
    // multiPart->append(modelPart);

    // Send request
    QUrl url(m_serverUrl + "/garments");
    QNetworkRequest request = createAuthenticatedRequest(url);
    
    qDebug() << "Sending upload request to:" << url.toString();
    
    QNetworkReply *reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply); // Delete multiPart with reply
    
    connect(reply, &QNetworkReply::finished, this, [this, reply/*, previewPath, modelPath*/]() {
        // Clean up temporary files if they were created
        // if (previewPath.contains(QStandardPaths::writableLocation(QStandardPaths::TempLocation))) {
        //     QFile::remove(previewPath);
        //     qDebug() << "Cleaned up temp preview file:" << previewPath;
        // }
        // if (modelPath.contains(QStandardPaths::writableLocation(QStandardPaths::TempLocation))) {
        //     QFile::remove(modelPath);
        //     qDebug() << "Cleaned up temp model file:" << modelPath;
        // }
        
        handleUploadFinished(reply);
    });
    
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply](QNetworkReply::NetworkError error) {
        qWarning() << "Upload error occurred:" << error << reply->errorString();
    });
    
    // Optional: connect upload progress
    connect(reply, &QNetworkReply::uploadProgress, this, [this](qint64 bytesSent, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            int progress = (int)((bytesSent * 100) / bytesTotal);
            qDebug() << "Upload progress:" << progress << "%";
            // You can emit a signal here if you want to show progress in UI
        }
    });
}
// ---- Delete Garment (DELETE /garments/:garmentId) ----
void NetworkManager::deleteGarment(const QString& garmentId) {
    QUrl url(m_serverUrl + "/garments/" + garmentId);
    QNetworkRequest request = createAuthenticatedRequest(url);
    QNetworkReply *reply = m_networkManager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, garmentId]() {
        if (reply->error() == QNetworkReply::NoError) {
            emit garmentDeleteSucceeded(garmentId);
        } else {
            emit garmentUploadFailed(reply->errorString());
        }
        reply->deleteLater();
    });
}


// Private slot implementation
void NetworkManager::handleUploadFinished(QNetworkReply* reply)
{
    QScopedPointer<QNetworkReply> replyDeleter(reply);  // Auto-delete

    bool ok = false;
    QJsonDocument response = parseJsonReply(reply, ok);

    if (reply->error() == QNetworkReply::NoError && ok) {
        if (response.isObject()) {
            QJsonObject obj = response.object();
            if (obj.contains("garmentId")) {
                QString garmentId = obj["garmentId"].toString();
                emit garmentUploadSucceeded(garmentId);
                emit networkRequestFinished();
                return;
            }
        }
        emit garmentUploadFailed(tr("Invalid response format"));
    } else {
        QString error = reply->errorString();
        if (!response.isNull() && response.isObject()) {
            error = response.object()["error"].toString(error);
        }
        emit garmentUploadFailed(error);
    }

    emit networkRequestFinished();
}


void NetworkManager::registerUser(const QString& username, const QString& email, const QString& password) {
    QUrl url(m_serverUrl + "/auth/register");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject userData{
        {"username", username},
        {"email", email},
        {"password", password}
    };

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(userData).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        handleAuthResponse(reply, true);
        reply->deleteLater();
    });
}

void NetworkManager::loginUser(const QString& email, const QString& password) {
    QUrl url(m_serverUrl + "/auth/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject credentials{
        {"email", email},
        {"password", password}
    };

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(credentials).toJson());
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        handleAuthResponse(reply, false);
        reply->deleteLater();
    });
}

void NetworkManager::logoutUser() {
    clearAuthToken();
    m_userId = QString();
    m_username = QString();
    emit userLoggedOut();
}

void NetworkManager::handleAuthResponse(QNetworkReply* reply, bool isRegistration) {
    bool ok;
    QJsonDocument response = parseJsonReply(reply, ok);
    
    qDebug() << "Auth response - HTTP Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "Auth response - Parse OK:" << ok;
    qDebug() << "Auth response - Network Error:" << reply->error();
    
    if(reply->error() == QNetworkReply::NoError && ok) {
        QJsonObject responseObj = response.object();
        
        qDebug() << "Auth response object:" << responseObj;
        
        if(responseObj.contains("token")) {
            m_authToken = responseObj["token"].toString();
            qDebug() << "Token received and saved:" << m_authToken.left(20) + "...";
            saveAuthToken(m_authToken);
            
            // Extract user data from login response
            if(responseObj.contains("user")) {
                QJsonObject userObj = responseObj["user"].toObject();
                m_userId = userObj["id"].toString();
                m_username = userObj["username"].toString();
                
                qDebug() << "User data extracted - ID:" << m_userId << "Username:" << m_username;
                
                emit userLoggedIn(m_username, m_userId);
                return; // Important: return here to avoid calling fetchUserData
            }
            
            // Fallback: fetch user data separately if not included in response
            if(!isRegistration) {
                qDebug() << "No user data in response, fetching separately...";
                fetchUserData();
            }
            else {
                emit registrationSucceeded(responseObj.value("username").toString());
            }
        } else {
            qDebug() << "No token in response";
            QString error = "No authentication token received";
            if(isRegistration) {
                emit registrationFailed(error);
            } else {
                emit authenticationFailed(error);
            }
        }
    }
    else {
        qDebug() << "Auth failed - Response data:" << reply->readAll();
        
        QString error = "Authentication failed";
        if(ok && response.object().contains("error")) {
            error = response.object().value("error").toString();
        } else if(reply->error() != QNetworkReply::NoError) {
            error = reply->errorString();
        }
        
        qDebug() << "Auth error:" << error;
        
        if(isRegistration) {
            emit registrationFailed(error);
        }
        else {
            emit authenticationFailed(error);
        }
    }
}

void NetworkManager::fetchUserData() {
    QNetworkRequest request = createAuthenticatedRequest(QUrl(m_serverUrl + "/auth/verify"));
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        bool ok;
        QJsonDocument response = parseJsonReply(reply, ok);
        
        qDebug() << "Fetch user data - HTTP Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "Fetch user data - Parse OK:" << ok;
        qDebug() << "Fetch user data response:" << response.object();
        
        if(reply->error() == QNetworkReply::NoError && ok) {
            QJsonObject responseObj = response.object();
            if(responseObj.contains("user")) {
                QJsonObject user = responseObj["user"].toObject();
                m_userId = user["id"].toString();
                m_username = user["username"].toString();
                
                qDebug() << "User data fetched - ID:" << m_userId << "Username:" << m_username;
                emit userLoggedIn(m_username, m_userId);
            } else {
                qDebug() << "No user data in verify response";
                emit authenticationFailed("Invalid user data received");
            }
        }
        else {
            qDebug() << "Failed to fetch user data:" << reply->errorString();
            emit authenticationFailed("Failed to fetch user data: " + reply->errorString());
        }
        reply->deleteLater();
    });
}


// Check if user is logged in
bool NetworkManager::isUserLoggedIn() const {
    return !m_authToken.isEmpty();
}

// Sync user data
void NetworkManager::syncUserData() {
    if (!isUserLoggedIn()) {
        emit networkError("Cannot sync data: User not logged in");
        return;
    }

    QNetworkRequest request = createAuthenticatedRequest(QUrl(m_serverUrl + "/user/sync"));

    emit networkRequestStarted();
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        bool ok;
        QJsonDocument response = parseJsonReply(reply, ok);

        if (ok && reply->error() == QNetworkReply::NoError) {
            // Handle sync response
            // This could trigger updating local data
            qDebug() << "User data synced successfully";
        } else {
            QString errorMsg = "Sync failed: " + reply->errorString();
            if (ok) {
                errorMsg = response.object().value("error").toString(errorMsg);
            }
            emit networkError(errorMsg);
        }

        reply->deleteLater();
        emit networkRequestFinished();
    });
}

// Check server status
void NetworkManager::checkServerStatus() {
    QNetworkRequest request(QUrl(m_serverUrl + "/api/status"));

    emit networkRequestStarted();
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        bool ok;
        QJsonDocument response = parseJsonReply(reply, ok);

        if (ok && reply->error() == QNetworkReply::NoError) {
            bool serverOnline = response.object().value("online").toBool(false);
            QString version = response.object().value("version").toString("unknown");

            qDebug() << "Server status: " << (serverOnline ? "Online" : "Offline")
                     << ", Version: " << version;
        } else {
            emit networkError("Server status check failed: " + reply->errorString());
        }

        reply->deleteLater();
        emit networkRequestFinished();
    });
}

// Handle authentication required
void NetworkManager::onAuthenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator) {
    // This could be used for HTTP Basic Auth if needed
    qDebug() << "Authentication required for:" << reply->url().toString();

    // If we have stored credentials, use them
    if (!m_authToken.isEmpty()) {
        authenticator->setUser("token");
        authenticator->setPassword(m_authToken);
    } else {
        // Otherwise, the request will likely fail
        qWarning() << "No authentication token available";
    }
}


void NetworkManager::processNetworkError(QNetworkReply::NetworkError error) {
    Q_UNUSED(error)
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) handleNetworkError(reply);
}

void NetworkManager::saveAuthToken(const QString& token) {
    QSettings settings;
    settings.beginGroup("Authentication");
    settings.setValue("token", token);
    settings.endGroup();
}

QString NetworkManager::loadAuthToken() {
    QSettings settings;
    settings.beginGroup("Authentication");
    return settings.value("token").toString();
}

void NetworkManager::clearAuthToken() {
    QSettings settings;
    settings.beginGroup("Authentication");
    settings.remove("token");
    settings.endGroup();
    m_authToken = QString();
}

QJsonDocument NetworkManager::parseJsonReply(QNetworkReply* reply, bool& ok) {
    ok = false;
    QByteArray data = reply->readAll();
    
    if(data.isEmpty()) {
        return QJsonDocument();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    ok = (parseError.error == QJsonParseError::NoError);
    
    return doc;
}

// Handle network errors
void NetworkManager::handleNetworkError(QNetworkReply* reply) {
    QString errorString = reply->errorString();
    QNetworkReply::NetworkError errorCode = reply->error();

    // Check if the error response contains JSON data
    QByteArray errorData = reply->readAll();
    if (!errorData.isEmpty()) {
        QJsonParseError parseError;
        QJsonDocument errorJson = QJsonDocument::fromJson(errorData, &parseError);

        if (parseError.error == QJsonParseError::NoError && errorJson.isObject()) {
            QString serverErrorMsg = errorJson.object().value("error").toString();
            if (!serverErrorMsg.isEmpty()) {
                errorString = serverErrorMsg;
            }
        }
    }

    qWarning() << "Network error:" << errorString << "(" << errorCode << ")";

    // Handle authentication errors specifically
    if (errorCode == QNetworkReply::AuthenticationRequiredError) {
        clearAuthToken();
        emit connectionStatusChanged(false); // Add this
        emit authenticationFailed("Authentication failed: " + errorString);
    }

    emit networkError(errorString);
}
