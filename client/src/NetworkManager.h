#pragma once
#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QSettings>
#include <QSslConfiguration>
#include <QtQml/qqmlregistration.h>
#include <QAuthenticator>
#include <QFileInfo>

class NetworkManager : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
    

public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    // // MongoDB connection methods
    // Q_INVOKABLE bool connectToDatabase(const QString& dbName,
    //                                    const QString& username = QString(),
    //                                    const QString& password = QString());
    // Q_INVOKABLE void disconnectFromDatabase();

    // Properties
    QString serverUrl() const;
    void setServerUrl(const QString& url);

    // CRUD operations
    Q_INVOKABLE void fetchGarments(bool forceRefresh = false);
    Q_INVOKABLE void uploadGarment(const QJsonObject& garmentData, 
                              const QString& previewPath = QString(),
                              const QString& modelPath = QString());
    Q_INVOKABLE void deleteGarment(const QString& garmentId);

    // User management
    Q_INVOKABLE void registerUser(const QString& username, const QString& email, const QString& password);
    Q_INVOKABLE void loginUser(const QString& email, const QString& password);
    Q_INVOKABLE void logoutUser();
    Q_INVOKABLE bool isUserLoggedIn() const;

    // Utility methods
    Q_INVOKABLE void syncUserData();
    Q_INVOKABLE void checkServerStatus();

signals:
    // Connection status
    void connectionStatusChanged(bool connected);
    void serverUrlChanged();
    void databaseConnected(const QString& dbName);
    void databaseDisconnected();
    void connectionError(const QString& error);

    // CRUD responses
    void garmentsReceived(const QJsonArray& garments);
    void garmentDetailsReceived(const QString& garmentId, const QJsonObject& details);
    void garmentUploadSucceeded(const QString& garmentId);
    void garmentUploadFailed(const QString& errorMessage);
    void garmentUpdateSucceeded(const QString& garmentId);
    void garmentDeleteSucceeded(const QString& garmentId);

    // Authentication signals
    void userLoggedIn(const QString& username, const QString& userId);
    void userLoggedOut();
    void registrationSucceeded(const QString& username);
    void authenticationFailed(const QString& reason);
    void registrationFailed(const QString& reason);

    // Network signals
    void networkRequestStarted();
    void networkRequestFinished();
    void networkError(const QString& errorMessage);

private slots:
    void onAuthenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator);
    void handleGarmentsResponse(QNetworkReply* reply);
    void handleAuthResponse(QNetworkReply* reply, bool isRegistration);
    void processNetworkError(QNetworkReply::NetworkError error);
    void handleUploadFinished(QNetworkReply* reply);
    void fetchUserData();
    bool isConnected() const { return !m_authToken.isEmpty(); }
    void verifyAuthToken();
    void verifyServerConnectivity();
#ifndef QT_NO_SSL
    void onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors);
#endif

private:
    // Network components
    QNetworkAccessManager* m_networkManager;
#ifndef QT_NO_SSL
    QSslConfiguration m_sslConfig;
#endif

    // Configuration
    QString m_serverUrl;
    QString m_databaseName;
    bool m_connected;

    // Authentication
    QString m_authToken;
    QString m_userId;
    QString m_username;

    // Helper methods
    QNetworkRequest createAuthenticatedRequest(const QUrl& url);
    void saveAuthToken(const QString& token);
    void clearAuthToken();
    QString loadAuthToken();
    QJsonDocument parseJsonReply(QNetworkReply* reply, bool& ok);
    void handleNetworkError(QNetworkReply* reply);


};

#endif // NETWORKMANAGER_H
