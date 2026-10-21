#include <QNetworkReply>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include "clientservice.h"
#include <QNetworkRequest> // describes one HTTP request
#include <Qurl>
#include <QDateTime>

ClientService::ClientService(QObject *parent)
    : QObject{parent}
{}

/*
 * Sends a registration request to the homeserver
 * Register button -> POST /register
 */
void ClientService::registerUser(const QString &username,
                                 const QString &password)
{
    QNetworkRequest request(QUrl("http://127.0.0.1:8080/register"));

    // tells HTTP request that body will contain JSON
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject data;
    data["username"] = username;
    data["password"] = password;

    // wrap JSON object in JSON document of bytes
    QByteArray jsonData = QJsonDocument(data).toJson(QJsonDocument::Compact);

    // Send post request and store homeserver's reply
    QNetworkReply *reply = networkManager.post(request, jsonData);

    // once homeserver finished responding run code
    // First 'this' lets Qt manage the connection
    // Second 'this' lets the lambda access the ClientService to emit registration signals
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString response = QString::fromUtf8(reply->readAll());
        if (statusCode == 200 || statusCode == 201){
            emit registrationSucceeded();
        } else{
            emit registrationFailed(response);
        }
        reply->deleteLater();
    });
}

/*
 * Sends a login request to the homeserver
 * Login button -> POST /login
 */
void ClientService::loginUser(const QString &username, const QString &password)
{
    QNetworkRequest request(QUrl("http://127.0.0.1:8080/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject data;
    data["username"] = username;
    data["password"] = password;

    QByteArray jsonData = QJsonDocument(data).toJson(QJsonDocument::Compact);

    QNetworkReply *reply = networkManager.post(request, jsonData);

    connect(reply, &QNetworkReply::finished, this, [this, reply, username](){
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString response = QString::fromUtf8(reply->readAll());
        if (statusCode == 200){
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8());
            if (jsonDoc.isObject()){
                QJsonObject jsonObj = jsonDoc.object();
                QString authToken = jsonObj["authToken"].toString();
                if (!authToken.isEmpty()){
                    this->authToken = authToken;
                    this->currentUsername = username;
                    emit loginSucceeded(authToken);
                } else{
                    emit loginFailed("Authentication token missing");
                }
            } else{
                emit loginFailed("Invalid server response");
            }
        } else {
            emit loginFailed(response);
        }
        reply->deleteLater();
    });
}

/*
 * Sends a request to the homeserver to start a chat with another user
 * Start chat -> POST /user/events
 */
void ClientService::startChat(const QString &recipient){
    QNetworkRequest request(QUrl("http://127.0.0.1:8080/user/events"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject data;
    data["authToken"] = authToken;
    data["recipient"] = recipient;
    data["sender"] = currentUsername;
    data["eventType"] = "start_chat";
    data["data"] = "";
    data["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QByteArray jsonData = QJsonDocument(data).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = networkManager.post(request, jsonData);
    connect(reply, &QNetworkReply::finished, this, [this,reply](){
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString response = QString::fromUtf8(reply->readAll());

        if(statusCode == 200 || statusCode == 201){
            emit chatStarted();
        } else{
            emit chatFailed(response.isEmpty() ? "Failed to start chat" : response);
        }
        reply->deleteLater();
    });
}

/*
 * Sends a message event to the homeserver
 * Send message -> POST /user/events
 */
void ClientService::sendMessage(const QString &recipient, const QString &message){
    QNetworkRequest request(QUrl("http://127.0.0.1:8080/user/events"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject data;
    data["authToken"] = authToken;
    data["recipient"] = recipient;
    data["sender"] = currentUsername;
    data["eventType"] = "message";
    data["data"] = message; // TO-DO: Encrypt Message
    data["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QByteArray jsonData = QJsonDocument(data).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = networkManager.post(request, jsonData);
    connect(reply, &QNetworkReply::finished, this, [this,reply](){
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString response = QString::fromUtf8(reply->readAll());

        if(statusCode == 200 || statusCode == 201){
            emit messageSent();
        } else{
            emit messageFailed(response.isEmpty() ? "Failed to send message" : response);
        }
        reply->deleteLater();
    });
}