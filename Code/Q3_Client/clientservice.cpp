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
                    // open this account's local storage before showing the chat screen
                    if (!openDatabase(username)){
                        emit loginFailed("Could not open local message storage");
                    } else{
                        this->authToken = authToken;
                        this->currentUsername = username;
                        emit loginSucceeded(authToken);
                    }
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
    connect(reply, &QNetworkReply::finished, this, [this,reply,recipient](){
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString response = QString::fromUtf8(reply->readAll());

        if(statusCode == 200 || statusCode == 201){
            // remember the recipient locally once the homeserver accepts the chat
            if (database && !database->addContact(recipient)){
                qWarning() << "Failed to store contact:" << database->lastError();
            }
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
    // save the message as pending before sending, so it is kept even if the request fails
    QDateTime sentAt = QDateTime::currentDateTimeUtc();
    std::optional<qint64> messageId;
    if (database){
        std::optional<qint64> contactId = database->addContact(recipient);
        if (contactId){
            messageId = database->addMessage(*contactId, MessageDirection::Outgoing, message,
                                             sentAt, MessageStatus::Pending);
        }
    }
    if (!messageId){
        qWarning() << "Failed to store message:" << (database ? database->lastError() : "not logged in");
        emit messageFailed("Could not save message locally");
        return;
    }

    QNetworkRequest request(QUrl("http://127.0.0.1:8080/user/events"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject data;
    data["authToken"] = authToken;
    data["recipient"] = recipient;
    data["sender"] = currentUsername;
    data["eventType"] = "message";
    data["data"] = message; // TO-DO: Encrypt Message
    data["timestamp"] = sentAt.toString(Qt::ISODate);

    QByteArray jsonData = QJsonDocument(data).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = networkManager.post(request, jsonData);
    connect(reply, &QNetworkReply::finished, this, [this,reply,messageId = *messageId](){
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString response = QString::fromUtf8(reply->readAll());

        if(statusCode == 200 || statusCode == 201){
            // keep the server's event ID (if returned) so a later sync won't duplicate this message
            std::optional<qint64> serverEventId;
            QJsonObject body = QJsonDocument::fromJson(response.toUtf8()).object();
            if (body.contains("id")){
                serverEventId = body["id"].toInteger();
            }
            database->updateMessageStatus(messageId, MessageStatus::Sent, serverEventId);
            emit messageSent();
        } else{
            database->updateMessageStatus(messageId, MessageStatus::Failed);
            emit messageFailed(response.isEmpty() ? "Failed to send message" : response);
        }
        reply->deleteLater();
    });
}

/*
 * Returns the stored messages with a recipient, oldest first
 * Each entry: {id, direction: "in"/"out", body, sentAt (Date), status: "pending"/"sent"/"failed"/"deleted"}
 */
QVariantList ClientService::conversationHistory(const QString &recipient, int limit){
    QVariantList history;
    if (!database){
        return history;
    }
    std::optional<StoredContact> contact = database->contact(recipient);
    if (!contact){
        return history; // no conversation with this recipient yet
    }
    std::optional<QList<StoredMessage>> messages = database->messages(contact->id, 0, limit);
    if (!messages){
        qWarning() << "Failed to load messages:" << database->lastError();
        return history;
    }
    for (const StoredMessage &message : *messages){
        QVariantMap entry;
        entry["id"] = message.id;
        entry["direction"] = ClientDatabase::toText(message.direction);
        entry["body"] = message.body;
        entry["sentAt"] = message.sentAt;
        entry["status"] = ClientDatabase::toText(message.status);
        history.append(entry);
    }
    return history;
}

/*
 * Opens the signed-in user's local database, replacing any previous account's
 */
bool ClientService::openDatabase(const QString &username){
    std::unique_ptr<ClientDatabase> userDatabase = std::make_unique<ClientDatabase>();
    if (!userDatabase->open(ClientDatabase::pathForUser(username))){
        qWarning() << "Failed to open local database:" << userDatabase->lastError();
        return false;
    }
    database = std::move(userDatabase);
    return true;
}