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

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)), // Parent properly set
      m_connected(false)
{
    #ifdef Q_OS_ANDROID
        m_serverUrl = "http://192.168.1.4:5000/api";
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
    if (!m_authToken.isEmpty()) {
        QNetworkRequest request = createAuthenticatedRequest(QUrl(m_serverUrl + "/auth/verify"));
        QNetworkReply* reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            bool ok;
            QJsonDocument jsonResponse = parseJsonReply(reply, ok);
            if (!ok || !jsonResponse.object().value("valid").toBool()) {
                clearAuthToken();
            }
            reply->deleteLater();
        });
    }
}

// Destructor
NetworkManager::~NetworkManager() {
    disconnectFromDatabase();
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

bool NetworkManager::connectToDatabase(const QString& dbName,
                                     const QString& username,
                                     const QString& password) {
    m_databaseName = dbName;
    QJsonObject connectData{{"database", dbName}};

    if (!username.isEmpty() && !password.isEmpty()) {
        connectData["username"] = username;
        connectData["password"] = password;
    }

    QNetworkRequest request(QUrl(m_serverUrl + "/database/connect"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    QJsonDocument jsonDoc(connectData);
    QNetworkReply* reply = m_networkManager->post(request, jsonDoc.toJson());

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        // lambda implementation
    });
    loop.exec();

    bool success = false;
    if (reply->error() == QNetworkReply::NoError) {
        bool ok;
        QJsonDocument response = parseJsonReply(reply, ok);
        if (ok && response.object()["success"].toBool()) {
            m_connected = true;
            emit connectionStatusChanged(true);
            emit databaseConnected(dbName);
            success = true;
        } else {
            emit connectionError(response.object()["error"].toString());
        }
    } else {
        handleNetworkError(reply);
    }

    reply->deleteLater();
    return success;
}

// Disconnect from database
void NetworkManager::disconnectFromDatabase() {
    if (!m_connected) {
        return;
    }

    QNetworkRequest request = createAuthenticatedRequest(QUrl(m_serverUrl + "/database/disconnect"));

    emit networkRequestStarted();
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_connected = false;
        emit connectionStatusChanged(false);
        emit databaseDisconnected();

        reply->deleteLater();
        emit networkRequestFinished();
    });
}

// Check if connected to database
bool NetworkManager::isConnected() const {
    return m_connected;
}

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
void NetworkManager::fetchGarments(bool forceRefresh) {
    if (!m_connected && !forceRefresh) {
        emit connectionError("Not connected to database");
        return;
    }

    QUrl url(m_serverUrl + "/garments");
    QUrlQuery query;
    if (forceRefresh) query.addQueryItem("refresh", "true");
    url.setQuery(query);

    QNetworkReply* reply = m_networkManager->get(createAuthenticatedRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleGarmentsResponse(reply);
    });
    connect(reply, &QNetworkReply::errorOccurred, this, &NetworkManager::processNetworkError);
}

// Handle response for garments fetching
void NetworkManager::handleGarmentsResponse(QNetworkReply* reply) {
    bool ok;
    QJsonDocument jsonResponse = parseJsonReply(reply, ok);

    if (ok && reply->error() == QNetworkReply::NoError) {
        QJsonArray garmentsArray = jsonResponse.object().value("garments").toArray();
        emit garmentsReceived(garmentsArray);
    } else {
        handleNetworkError(reply);
    }

    reply->deleteLater();
    emit networkRequestFinished();
}

// Upload a new garment
void NetworkManager::uploadGarment(const QJsonObject& garmentData,
                                 const QString& previewPath,
                                 const QString& modelPath)
{
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QNetworkRequest request = createAuthenticatedRequest(QUrl(m_serverUrl + "/garments"));

    try {
        QHttpPart metadataPart;
        metadataPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        metadataPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                             "form-data; name=\"metadata\"");
        metadataPart.setBody(QJsonDocument(garmentData).toJson());
        multiPart->append(metadataPart);

        auto addFilePart = [multiPart](const QString& path, const QString& partName) {
            if (path.isEmpty()) return;

            QFile* file = new QFile(path);
            if (!file->open(QIODevice::ReadOnly)) {
                throw std::runtime_error(QString("Could not open %1 file").arg(partName).toStdString());
            }

            QHttpPart filePart;
            filePart.setHeader(QNetworkRequest::ContentTypeHeader,
                             QMimeDatabase().mimeTypeForFile(path).name());
            filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                QString("form-data; name=\"%1\"; filename=\"%2\"")
                    .arg(partName)
                    .arg(QFileInfo(path).fileName()));
            filePart.setBodyDevice(file);
            multiPart->append(filePart);
        };

        addFilePart(previewPath, "preview");
        addFilePart(modelPath, "model");

        QNetworkReply* reply = m_networkManager->post(request, multiPart);
        multiPart->setParent(reply);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            handleUploadFinished(reply);
        });
        connect(reply, &QNetworkReply::errorOccurred, this, &NetworkManager::processNetworkError);

    } catch (const std::exception& e) {
        multiPart->deleteLater();
        emit garmentUploadFailed(QString::fromStdString(e.what()));
    }
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

// Delete a garment
void NetworkManager::deleteGarment(const QString& garmentId) {
    if (!m_connected) {
        emit connectionError("Not connected to database");
        return;
    }

    QUrl url(m_serverUrl + "/garments/" + garmentId);
    QNetworkRequest request = createAuthenticatedRequest(url);

    emit networkRequestStarted();
    QNetworkReply* reply = m_networkManager->deleteResource(request);

    connect(reply, &QNetworkReply::finished, this, [this, garmentId, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            emit garmentDeleteSucceeded(garmentId);
        } else {
            emit networkError("Delete failed: " + reply->errorString());
        }

        reply->deleteLater();
        emit networkRequestFinished();
    });

    // Fixed: Use QOverload to specify the correct errorOccurred signal
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &NetworkManager::processNetworkError);
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
    
    if(reply->error() == QNetworkReply::NoError && ok) {
        QJsonObject responseObj = response.object();
        if(responseObj.contains("token")) {
            m_authToken = responseObj["token"].toString();
            saveAuthToken(m_authToken);
            
            // For login, fetch additional user data
            if(!isRegistration) {
                fetchUserData();
            }
            else {
                emit registrationSucceeded(responseObj.value("username").toString());
            }
        }
    }
    else {
        QString error = response.object().value("error").toString();
        if(error.isEmpty()) error = "Authentication failed";
        
        if(isRegistration) {
            emit registrationFailed(error);
        }
        else {
            emit authenticationFailed(error);
        }
    }
}

void NetworkManager::fetchUserData() {
    QNetworkRequest request = createAuthenticatedRequest(QUrl(m_serverUrl + "/auth/me"));
    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        bool ok;
        QJsonDocument response = parseJsonReply(reply, ok);
        
        if(reply->error() == QNetworkReply::NoError && ok) {
            QJsonObject user = response.object();
            m_userId = user["_id"].toString();
            m_username = user["username"].toString();
            emit userLoggedIn(m_username, m_userId);
        }
        else {
            emit authenticationFailed("Failed to fetch user data");
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
    QNetworkRequest request(QUrl(m_serverUrl + "/status"));

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
        emit authenticationFailed("Authentication failed: " + errorString);
    }

    emit networkError(errorString);
}