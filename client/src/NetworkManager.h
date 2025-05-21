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

    // Properties for QML
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)

public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    // MongoDB connection methods
    Q_INVOKABLE bool connectToDatabase(const QString& dbName,
                                       const QString& username = QString(),
                                       const QString& password = QString());
    Q_INVOKABLE void disconnectFromDatabase();

    // Properties getters/setters
    bool isConnected() const;
    QString serverUrl() const;
    void setServerUrl(const QString& url);

    // CRUD operations for garments collection
    Q_INVOKABLE void fetchGarments(bool forceRefresh = false);
    Q_INVOKABLE void fetchGarmentDetails(const QString& garmentId);
    Q_INVOKABLE void uploadGarment(const QJsonObject& garmentData, const QString& modelPath);
    Q_INVOKABLE void updateGarment(const QString& garmentId, const QJsonObject& garmentData);
    Q_INVOKABLE void deleteGarment(const QString& garmentId);

    // User management
    Q_INVOKABLE void registerUser(const QString& username, const QString& email, const QString& password);
    Q_INVOKABLE void loginUser(const QString& email, const QString& password);
    Q_INVOKABLE void logoutUser();
    Q_INVOKABLE bool isUserLoggedIn() const;

    // Additional utility methods
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

    // General network signals
    void networkRequestStarted();
    void networkRequestFinished();
    void networkError(const QString& errorMessage);

private slots:
    void onAuthenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator);
    void onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors);
    void handleGarmentsResponse(QNetworkReply* reply);
    void handleGarmentDetailsResponse(QNetworkReply* reply);
    void handleAuthResponse(QNetworkReply* reply);
    void processNetworkError(QNetworkReply::NetworkError error);

private:
    // Network components
    QNetworkAccessManager* m_networkManager;
    QSslConfiguration m_sslConfig;

    // Server/database configuration
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
