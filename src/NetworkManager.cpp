#include "NetworkManager.h"
#include <QAuthenticator>
#include <QDebug>
#include <QFile>
#include <QHttpMultiPart>
#include <QMimeDatabase>
#include <QSslSocket>
#include <QStandardPaths>

// Constructor
NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_connected(false)
    , m_serverUrl("https://api.yourvirtualfittingapp.com/v1") // Default server URL - replace with your actual API endpoint
{
    // Configure SSL
    m_sslConfig = QSslConfiguration::defaultConfiguration();
    m_sslConfig.setProtocol(QSsl::TlsV1_2OrLater);

    // Connect network manager signals
    connect(m_networkManager, &QNetworkAccessManager::authenticationRequired,
            this, &NetworkManager::onAuthenticationRequired);
    connect(m_networkManager, &QNetworkAccessManager::sslErrors,
            this, &NetworkManager::onSslErrors);

    // Load authentication token if available
    m_authToken = loadAuthToken();
    if (!m_authToken.isEmpty()) {
        // Verify token validity at startup (optional)
        // This could be implemented by making a lightweight API call
        QNetworkRequest request = createAuthenticatedRequest(QUrl(m_serverUrl + "/auth/verify"));
        QNetworkReply* reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, [this, reply]() {
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

// Connect to MongoDB database
bool NetworkManager::connectToDatabase(const QString& dbName,
                                       const QString& username,
                                       const QString& password) {
    // For MongoDB Atlas or other cloud-based services, we may not need to explicitly connect
    // but we can validate our connection parameters
    m_databaseName = dbName;

    QJsonObject connectData;
    connectData["database"] = dbName;

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
    QByteArray jsonData = jsonDoc.toJson();

    emit networkRequestStarted();
    QNetworkReply* reply = m_networkManager->post(request, jsonData);

    // Use a blocking connection for the initial setup
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool success = false;
    if (reply->error() == QNetworkReply::NoError) {
        bool ok;
        QJsonDocument response = parseJsonReply(reply, ok);

        if (ok && response.object().value("success").toBool()) {
            m_connected = true;
            emit connectionStatusChanged(true);
            emit databaseConnected(dbName);
            success = true;
        } else {
            QString errorMsg = response.object().value("error").toString("Unknown database connection error");
            emit connectionError(errorMsg);
            qWarning() << "Database connection error:" << errorMsg;
        }
    } else {
        handleNetworkError(reply);
    }

    reply->deleteLater();
    emit networkRequestFinished();

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

    connect(reply, &QNetworkReply::finished, [this, reply]() {
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

    if (forceRefresh) {
        query.addQueryItem("refresh", "true");
    }

    url.setQuery(query);
    QNetworkRequest request = createAuthenticatedRequest(url);

    emit networkRequestStarted();
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleGarmentsResponse(reply);
    });

    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &NetworkManager::processNetworkError);
}

// Fetch details for a specific garment
void NetworkManager::fetchGarmentDetails(const QString& garmentId) {
    if (!m_connected) {
        emit connectionError("Not connected to database");
        return;
    }

    QUrl url(m_serverUrl + "/garments/" + garmentId);
    QNetworkRequest request = createAuthenticatedRequest(url);

    emit networkRequestStarted();
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleGarmentDetailsResponse(reply);
    });

    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &NetworkManager::processNetworkError);
}

// Upload a new garment
void NetworkManager::uploadGarment(const QJsonObject& garmentData, const QString& modelPath) {
    if (!m_connected) {
        emit connectionError("Not connected to database");
        return;
    }

    // Create multipart message for file upload
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Add JSON metadata
    QHttpPart jsonPart;
    jsonPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    jsonPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"metadata\""));
    jsonPart.setBody(QJsonDocument(garmentData).toJson());
    multiPart->append(jsonPart);

    // Add 3D model file
    if (!modelPath.isEmpty()) {
        QFile* file = new QFile(modelPath);
        if (file->open(QIODevice::ReadOnly)) {
            QHttpPart modelPart;

            // Determine MIME type
            QMimeDatabase db;
            QString mimeType = db.mimeTypeForFile(modelPath).name();

            modelPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(mimeType));
            modelPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                                QVariant("form-data; name=\"model\"; filename=\"" +
                                         QFileInfo(modelPath).fileName() + "\""));
            modelPart.setBodyDevice(file);
            file->setParent(multiPart); // multiPart will take ownership and delete file
            multiPart->append(modelPart);
        } else {
            delete file;
            delete multiPart;
            emit garmentUploadFailed("Could not open model file: " + modelPath);
            return;
        }
    }

    // Create request
    QUrl url(m_serverUrl + "/garments");
    QNetworkRequest request = createAuthenticatedRequest(url);

    emit networkRequestStarted();
    QNetworkReply* reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply); // reply will take ownership and delete multiPart

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        bool ok;
        QJsonDocument response = parseJsonReply(reply, ok);

        if (ok && reply->error() == QNetworkReply::NoError) {
            QString garmentId = response.object().value("garmentId").toString();
            emit garmentUploadSucceeded(garmentId);
        } else {
            QString errorMsg = "Upload failed: " + reply->errorString();
            if (ok) {
                errorMsg = response.object().value("error").toString(errorMsg);
            }
            emit garmentUploadFailed(errorMsg);
        }

        reply->deleteLater();
        emit networkRequestFinished();
    });

    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &NetworkManager::processNetworkError);
}

// Update an existing garment
void NetworkManager::updateGarment(const QString& garmentId, const QJsonObject& garmentData) {
    if (!m_connected) {
        emit connectionError("Not connected to database");
        return;
    }

    QUrl url(m_serverUrl + "/garments/" + garmentId);
    QNetworkRequest request = createAuthenticatedRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument jsonDoc(garmentData);
    QByteArray jsonData = jsonDoc.toJson();

    emit networkRequestStarted();
    QNetworkReply* reply = m_networkManager->put(request, jsonData);

    connect(reply, &QNetworkReply::finished, [this, garmentId, reply]() {
        bool ok;
        QJsonDocument response = parseJsonReply(reply, ok);

        if (ok && reply->error() == QNetworkReply::NoError) {
            emit garmentUpdateSucceeded(garmentId);
        } else {
            QString errorMsg = "Update failed: " + reply->errorString();
            if (ok) {
                errorMsg = response.object().value("error").toString(errorMsg);
            }
            emit networkError(errorMsg);
        }

        reply->deleteLater();
        emit networkRequestFinished();
    });

    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &NetworkManager::processNetworkError);
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

    connect(reply, &QNetworkReply::finished, [this, garmentId, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            emit garmentDeleteSucceeded(garmentId);
        } else {
            emit networkError("Delete failed: " + reply->errorString());
        }

        reply->deleteLater();
        emit networkRequestFinished();
    });

    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &NetworkManager::processNetworkError);
}

// Register a new user
void NetworkManager::registerUser(const QString& username, const QString& email, const QString& password) {
    QJsonObject registerData;
    registerData["username"] = username;
    registerData["email"] = email;
    registerData["password"] = password;

    QNetworkRequest request(QUrl(m_serverUrl + "/auth/register"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument jsonDoc(registerData);
    QByteArray jsonData = jsonDoc.toJson();

    emit networkRequestStarted();
    QNetworkReply* reply = m_networkManager->post(request, jsonData);

    connect(reply, &QNetworkReply::finished, [this, username, reply]() {
        handleAuthResponse(reply);
    });

    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &NetworkManager::processNetworkError);
}

// Login user
void NetworkManager::loginUser(const QString& email, const QString& password) {
    QJsonObject loginData;
    loginData["email"] = email;
    loginData["password"] = password;

    QNetworkRequest request(QUrl(m_serverUrl + "/auth/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument jsonDoc(loginData);
    QByteArray jsonData = jsonDoc.toJson();

    emit networkRequestStarted();
    QNetworkReply* reply = m_networkManager->post(request, jsonData);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        handleAuthResponse(reply);
    });

    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &NetworkManager::processNetworkError);
}

// Logout user
void NetworkManager::logoutUser() {
    if (m_authToken.isEmpty()) {
        emit userLoggedOut();
        return;
    }

    QNetworkRequest request = createAuthenticatedRequest(QUrl(m_serverUrl + "/auth/logout"));

    emit networkRequestStarted();
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        clearAuthToken();
        m_userId = QString();
        m_username = QString();

        emit userLoggedOut();
        reply->deleteLater();
        emit networkRequestFinished();
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

    connect(reply, &QNetworkReply::finished, [this, reply]() {
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

    connect(reply, &QNetworkReply::finished, [this, reply]() {
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

// Handle SSL errors
void NetworkManager::onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors) {
    QString errorString;
    for (const QSslError& error : errors) {
        if (!errorString.isEmpty()) {
            errorString += ", ";
        }
        errorString += error.errorString();
    }

    qWarning() << "SSL errors occurred:" << errorString;

    // In a development environment, you might want to ignore certain SSL errors
    // Be careful in production!
    // reply->ignoreSslErrors();

    emit networkError("SSL error: " + errorString);
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

// Handle response for garment details
void NetworkManager::handleGarmentDetailsResponse(QNetworkReply* reply) {
    bool ok;
    QJsonDocument jsonResponse = parseJsonReply(reply, ok);

    if (ok && reply->error() == QNetworkReply::NoError) {
        QJsonObject garmentDetails = jsonResponse.object().value("garment").toObject();
        QString garmentId = garmentDetails.value("_id").toString();
        emit garmentDetailsReceived(garmentId, garmentDetails);
    } else {
        handleNetworkError(reply);
    }

    reply->deleteLater();
    emit networkRequestFinished();
}

// Handle authentication responses (login/register)
void NetworkManager::handleAuthResponse(QNetworkReply* reply) {
    bool ok;
    QJsonDocument jsonResponse = parseJsonReply(reply, ok);
    QString requestPath = reply->url().path();

    if (ok && reply->error() == QNetworkReply::NoError) {
        QJsonObject responseObj = jsonResponse.object();

        if (requestPath.endsWith("/login")) {
            // Handle login response
            if (responseObj.contains("token")) {
                QString token = responseObj.value("token").toString();
                saveAuthToken(token);

                m_userId = responseObj.value("userId").toString();
                m_username = responseObj.value("username").toString();

                emit userLoggedIn(m_username, m_userId);
            } else {
                emit authenticationFailed("No token received from server");
            }
        } else if (requestPath.endsWith("/register")) {
            // Handle registration response
            if (responseObj.value("success").toBool()) {
                QString username = responseObj.value("username").toString();
                emit registrationSucceeded(username);

                // Some APIs might also return a token upon registration
                if (responseObj.contains("token")) {
                    QString token = responseObj.value("token").toString();
                    saveAuthToken(token);

                    m_userId = responseObj.value("userId").toString();
                    m_username = username;

                    emit userLoggedIn(m_username, m_userId);
                }
            } else {
                emit registrationFailed(responseObj.value("error").toString("Unknown registration error"));
            }
        }
    } else {
        QString errorMsg = reply->errorString();
        if (ok) {
            errorMsg = jsonResponse.object().value("error").toString(errorMsg);
        }

        if (requestPath.endsWith("/login")) {
            emit authenticationFailed(errorMsg);
        } else if (requestPath.endsWith("/register")) {
            emit registrationFailed(errorMsg);
        } else {
            emit networkError(errorMsg);
        }
    }

    reply->deleteLater();
    emit networkRequestFinished();
}

// Process network errors
void NetworkManager::processNetworkError(QNetworkReply::NetworkError error) {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        handleNetworkError(reply);
    }
}

// Create authenticated request
QNetworkRequest NetworkManager::createAuthenticatedRequest(const QUrl& url) {
    QNetworkRequest request(url);
    request.setSslConfiguration(m_sslConfig);

    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    return request;
}

// Save authentication token
void NetworkManager::saveAuthToken(const QString& token) {
    QSettings settings;
    settings.beginGroup("Authentication");
    settings.setValue("token", token);
    settings.endGroup();
    settings.sync();

    m_authToken = token;
}

// Clear authentication token
void NetworkManager::clearAuthToken() {
    QSettings settings;
    settings.beginGroup("Authentication");
    settings.remove("token");
    settings.endGroup();
    settings.sync();

    m_authToken = QString();
}

// Load authentication token
QString NetworkManager::loadAuthToken() {
    QSettings settings;
    settings.beginGroup("Authentication");
    QString token = settings.value("token").toString();
    settings.endGroup();

    return token;
}

// Parse JSON reply
QJsonDocument NetworkManager::parseJsonReply(QNetworkReply* reply, bool& ok) {
    ok = false;
    QByteArray data = reply->readAll();

    if (data.isEmpty()) {
        return QJsonDocument();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return QJsonDocument();
    }

    ok = true;
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
