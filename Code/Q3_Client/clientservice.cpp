#include <QNetworkReply>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include "clientservice.h"
#include <QNetworkRequest> // describes one HTTP request
#include <Qurl>

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
    connect(reply, &QNetworkReply::finished, [reply]() {
        qDebug() << "Server response:" << reply->readAll();
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

    connect(reply, &QNetworkReply::finished, [reply](){
        qDebug() << "Server response:" << reply->readAll();
        reply->deleteLater();
    });
}
