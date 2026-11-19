#include <QNetworkReply>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "clientservice.h"
#include <QNetworkRequest> // describes one HTTP request
#include <Qurl>
#include <QDateTime>

// Longest we ever wait between reconnect attempts
static constexpr int MaxReconnectDelayMs = 30000;

ClientService::ClientService(QObject *parent)
    : QObject{parent}
{
    // When the live connection opens, tell the server who we are.
    // The server drops connections that don't authenticate quickly.
    connect(&liveSocket, &QWebSocket::connected, this, [this](){
        QJsonObject auth;
        auth["type"] = "auth";
        auth["authToken"] = authToken;

        liveSocket.sendTextMessage(QString::fromUtf8(
            QJsonDocument(auth).toJson(QJsonDocument::Compact)));
    });

    // Every message the server pushes down the live connection goes to
    // handleLiveMessage
    connect(&liveSocket, &QWebSocket::textMessageReceived,
                this, &ClientService::handleLiveMessage);

    // The retry timer fires once per attempt. When it fires, try to connect.
    reconnectTimer.setSingleShot(true);
    connect(&reconnectTimer, &QTimer::timeout,
            this, &ClientService::connectLive);

    // If the live connection closes, or an attempt to open it fails, schedule
    // a retry. A failed attempt (for example, the server is down) may report
    // an error without a clean "disconnected", so we listen for both.
    connect(&liveSocket, &QWebSocket::disconnected,
            this, &ClientService::scheduleReconnect);
    connect(&liveSocket, &QWebSocket::errorOccurred,
            this, &ClientService::scheduleReconnect);
}

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
                        connectLive(); // Login worked, so open the live connection for pushed events
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
 * Loads the logged-in user's event queue from the homeserver.
 * GET /user/events
 */
void ClientService::loadEvents()
{
    if (authToken.isEmpty() || currentUsername.isEmpty()) {
        emit eventsLoadFailed("Missing login information");
        return;
    }

    QNetworkRequest request(
        QUrl("http://127.0.0.1:8080/user/events"));

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/json");

    QJsonObject data;
    data["username"] = currentUsername;
    data["authToken"] = authToken;

    QByteArray jsonData =
        QJsonDocument(data).toJson(QJsonDocument::Compact);

    QNetworkReply *reply = networkManager.sendCustomRequest(
        request,
        QByteArrayLiteral("GET"),
        jsonData);

    connect(reply, &QNetworkReply::finished, this,[this, reply]() {
        const int statusCode = reply->attribute(
                                        QNetworkRequest::HttpStatusCodeAttribute
                                        ).toInt();

        const QByteArray response = reply->readAll();

        if (statusCode == 204
            || (statusCode == 200
                && response.trimmed().isEmpty())) {
            emit eventsLoaded(QVariantList{});
            reply->deleteLater();
            return;
        }

        if (statusCode != 200) {
            const QString message =
                QString::fromUtf8(response);

            emit eventsLoadFailed(
                message.isEmpty()
                    ? "Failed to load events"
                    : message);

            reply->deleteLater();
            return;
        }

        const QJsonDocument document =
            QJsonDocument::fromJson(response);

        if (!document.isArray()) {
            emit eventsLoadFailed(
                "Invalid event queue response");
            reply->deleteLater();
            return;
        }

        QVariantList events;

        for (const QJsonValue &value : document.array()) {
            if (!value.isObject())
                continue;

            const QJsonObject wrapper = value.toObject();
            QJsonObject event;

            // Homeserver returns { "id": ..., "body": {...} }.
            if (wrapper.value("body").isObject()) {
                event = wrapper.value("body").toObject();

                if (wrapper.contains("id"))
                    event["id"] = wrapper.value("id");
            } else {
                event = wrapper;
            }

            events.append(event.toVariantMap());
        }

        emit eventsLoaded(events);
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

/*
 * Opens the live WebSocket connection to the homeserver
 *
 * The connection stays open so the server can push new events to us that
 * moment they happen. This only starts the connection and returns immediately.
 * The socket's connected() signal firess once it is ready.
 */
void ClientService::connectLive()
{
    // Connection should be kept alive. If it drops, the
    // reconnect logic will retry.
    wantLive = true;

    // Don't dial again if already connected or an attempt is in
    // progress.
    if (liveSocket.state() != QAbstractSocket::UnconnectedState) {
        return;
    }

    // The homeserer uses port 8080 for HTTP and port 8081 for live
    // WebSockets connections. "ws://" is the WebSocket version of "http://"
    liveSocket.open(QUrl("ws://127.0.0.1:8081"));
}

/*
 * Handles one message pushed by the homeserver on the live socket.
 *
 * Every message is a JSON object with a "type" field:
 *      "authenticated" -> the server accepted our token
 *      "event" -> a new event (chat message/request) for us
 *      "error" -> server rejected something and will close socket
 */
void ClientService::handleLiveMessage(const QString &text)
{
    const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
    if (!doc.isObject()){
        return; // not JSON we understand, so ignore it
    }

    const QJsonObject message = doc.object();
    const QString type = message.value("type").toString();

    if (type == "authenticated"){
        // The connection is healthy again, so the next failure starts from a
        // short wait instead of the long one.
        reconnectDelayMs = 1000;
        qDebug() << "Live connection authenticated";
    } else if (type == "event"){
        processEvent(message.value("id").toInteger(),
                     message.value("body").toObject());
    } else if (type == "error") {
        qWarning() << "Live connection error:" << message.value("message").toString();
    }
}

/*
 * Applies one event from the homeserver. Stores it locally and tells the UI.
 */
void ClientService::processEvent(qint64 id, const QJsonObject &body)
{
    if(!database) {
        return; // Not logged in, so there is no local database to store into
    }

    const QString sender = body.value("sender").toString();
    const QString eventType = body.value("eventType").toString();

    // ensures we don't store our messages twice if server sends ours back to us
    if(sender.compare(currentUsername, Qt::CaseInsensitive) == 0){
        return;
    }

    if(eventType == "message"){
        QDateTime sentAt = QDateTime::fromString(body.value("timestamp").toString(), Qt::ISODate);
        if(!sentAt.isValid()){
            sentAt = QDateTime::currentDateTimeUtc();
        }
        const QString text = body.value("data").toString(); //TO-DO: Decrypt message

        // Make sure sender is in our contacts, then store message
        const std::optional<qint64> contactId = database->addContact(sender);
        if(!contactId || !database->addMessage(*contactId, MessageDirection::Incoming,
                                                  text, sentAt, MessageStatus::Sent, id)){
            qWarning() << "Failed to store incoming message:" << database ->lastError();
            return;
        }
        emit messageReceived(sender, text, sentAt);
    }
}

/*
 * Schedules another connection attempt after a delay that doubles each time
 * (1s, 2s, 4s ... up to MaxReconnectDelayMs).
 *
 * Does nothing if we are not supposed to be connected, or if an attempt is
 * already scheduled.
 */
void ClientService::scheduleReconnect()
{
    if (!wantLive || reconnectTimer.isActive()) {
        return;
    }
    qDebug() << "Live connection lost; retrying in" << reconnectDelayMs << "ms";
    reconnectTimer.start(reconnectDelayMs);
    reconnectDelayMs = qMin(reconnectDelayMs * 2, MaxReconnectDelayMs);
}

