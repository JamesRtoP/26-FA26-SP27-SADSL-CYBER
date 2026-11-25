/*
 * Simplle homeserver to accept and respond to GET requests
 */
#include "logger.h"
#include "logmodel.h"

#include <QDir>
#include <QQmlContext>
#include <QStandardPaths>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QHttpServer>
#include <QTcpServer>
#include <QHostAddress>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>

#include "database.h"
#include "passwordhasher.h"
#include "demoUser.h"
#include "pushserver.h"

QString healthCheck(){
    return "OK";
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

// Log file goes in homeserver/logs inside the source folder.
    const QString logDir = QString(HOMESERVER_SOURCE_DIR) + "/logs";
    QDir().mkpath(logDir);
    const QString logPath = logDir + "/homeserver.log";

    // The window reads logModel; routes call logger.log(...).
    LogModel logModel;
    Logger logger(logModel, logPath);
    qInfo().noquote() << "Log file:" << logPath;

    QHttpServer server; // handles the HTTP requests
    QTcpServer tcpServer; // listens on the port
    Database database; // events are stored in PostgreSQL
    PushServer pushServer(database); // live updates over WebSocket, separate from the HTTP server

    if(!database.open())
    {
        qCritical() << "Failed to connect to the database:" << database.lastError();
        return -1;
    }

    std::vector<DemoUser> userList;

    bool listening = tcpServer.listen(QHostAddress:: Any, 8080); // listen on any interface
    if (!listening){
        qDebug() << "Server failed to start:" << tcpServer.errorString();
    }
    else{
        qDebug() << "Listening on port" << tcpServer.serverPort();
    }

    // Start the WebSocket server on port 8081 (the HTTP server uses 8080).
    // Clients will connect here and keep the connection open to receive new messages.
    if (!pushServer.listen(8081)) {
        qCritical() << "Push server failed to start on port 8081";
    } else {
        qDebug() << "Push server listening on port 8081";
    }

    // When someone requests /health, run healthCheck()
    server.route("/health", healthCheck);

    server.route(
        "/user/events",
        QHttpServerRequest::Method::Get,
        [&database](const QHttpServerRequest &request)
        {
            const QJsonDocument document =
                QJsonDocument::fromJson(request.body());

            if (!document.isObject()) {
                return QHttpServerResponse(
                    "Invalid JSON payload",
                    QHttpServerResponse::StatusCode::BadRequest);
            }

            const QJsonObject object = document.object();
            const QString username =
                object.value("username").toString();
            const QString authToken =
                object.value("authToken").toString();

            const std::optional<qint64> userId =
                database.sessionUser(authToken.toUtf8());

            const std::optional<StoredUser> storedUser =
                database.user(username);

            if (!userId
                || !storedUser
                || *userId != storedUser->id) {
                return QHttpServerResponse(
                    "Authentication Mismatch",
                    QHttpServerResponse::StatusCode::BadRequest);
            }

            // Optional cursor: the client sends the highest event ID it already
            // has, and we return only newer events. If it is missing, we start
            // from the beginning, so older clients keep working.
            const qint64 afterId =
                qMax<qint64>(0, object.value("afterId").toInteger(0));

            const std::optional<QList<StoredEvent>> newEvents =
                database.eventsForUser(*userId, afterId);

            if (!newEvents) {
                return QHttpServerResponse(
                    "Failed to retrieve events",
                    QHttpServerResponse::
                    StatusCode::InternalServerError);
            }

            QJsonArray jsonArray;

            for (const StoredEvent &event : *newEvents) {
                QJsonObject eventObject;
                eventObject["id"] = event.id;
                eventObject["body"] = event.body;
                jsonArray.append(eventObject);
            }

            return QHttpServerResponse(
                jsonArray,
                QHttpServerResponse::StatusCode::Ok);
        });

    // Sending a message or starting a chat: the client POSTs an event here.
    server.route("/user/events", QHttpServerRequest::Method::Post, [&database, &pushServer, &logger](const QHttpServerRequest &request)
    {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if(!doc.isObject())
        {
            return QHttpServerResponse("Invalid JSON payload", QHttpServerResponse::StatusCode::BadRequest);
        }
        QJsonObject event = doc.object();

        // The client includes its session token in the request. That token is a password
        // for its account, so it must never be stored with the event or sent to the
        // recipient. Remove it before doing anything else with the event.
        event.remove("authToken");

        QList<qint64> userIDs;
        userIDs.append(database.user(event.value("sender").toString())->id);
        std::optional<StoredUser> storedUser = database.user(event.value("recipient").toString());
        if(!storedUser.has_value())
        {
            return QHttpServerResponse("Recipient Does Not Exist", QHttpServerResponse::StatusCode::InternalServerError);
        }
        userIDs.append(storedUser->id);

        // Store the event once and add it to both the sender's and recipient's inboxes.
        std::optional<qint64> id = database.addEventForUsers(event, userIDs);
        if(!id)
        {
            qWarning() << "Failed to store event:" << database.lastError();
            return QHttpServerResponse("Failed to store event", QHttpServerResponse::StatusCode::InternalServerError);
        }

        // Tell any recipient who is connected right now. The event is already saved,
        // so anyone who is offline will still get it later.
        pushServer.pushEvent(userIDs, *id, event);

        // Describe what the event did instead of logging the raw request.
        const QString sender = event.value("sender").toString();
        const QString recipient = event.value("recipient").toString();
        const QString type = event.value("eventType").toString();

        if (type == "message")
            logger.log(QString("%1 sent a message to %2").arg(sender, recipient));
        else if (type == "start_chat")
            logger.log(QString("%1 started a chat with %2").arg(sender, recipient));
        else
            logger.log(QString("%1 sent a %2 event to %3").arg(sender, type, recipient));
        return QHttpServerResponse(QJsonObject{{"id", *id}}, QHttpServerResponse::StatusCode::Created);
    });

    // temporary test route for registration
    server.route("/register", QHttpServerRequest::Method::Post, [&database, &logger](const QHttpServerRequest &request)
    {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        QJsonObject obj = doc.object();
        QString username = obj["username"].toString();

        if (database.user(username))
        {
            return QHttpServerResponse("Username Taken", QHttpServerResponse::StatusCode::BadRequest);
        }


        QString password = obj["password"].toString();
        std::vector<unsigned char> saltVector = PasswordHasher::salt();
        std::vector<unsigned char> hashVector = PasswordHasher::scryptHash(password, saltVector);
        QByteArray salt((const char*)saltVector.data(), saltVector.size());
        QByteArray passwordHash((const char*)hashVector.data(), hashVector.size());
        std::optional<qint64> userId = database.createUser(username, passwordHash, salt);

        if (!userId)
        {
            qWarning() << "Failed to create user:" << database.lastError();
            return QHttpServerResponse("Failed to create user", QHttpServerResponse::StatusCode::InternalServerError);
        }
        logger.log(QString("%1 registered").arg(username));
        return QHttpServerResponse("User Created", QHttpServerResponse::StatusCode::Created);
    });

    server.route("/login", QHttpServerRequest::Method::Post, [&database, &logger](const QHttpServerRequest &request)
    {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        QJsonObject obj = doc.object();
        QString username = obj["username"].toString();
        QString password = obj["password"].toString();

        std::optional<StoredUser> storedUser = database.user(username);
        if (!storedUser)
        {
            logger.log(QString("Failed login attempt for %1").arg(username));
            return QHttpServerResponse("Incorrect Username or Password", QHttpServerResponse::StatusCode::BadRequest);
        }

        std::vector<unsigned char> saltVec(storedUser->salt.begin(), storedUser->salt.end());
        std::vector<unsigned char> computedHashVec = PasswordHasher::scryptHash(password, saltVec);
        QByteArray computedHash((const char*)computedHashVec.data(), computedHashVec.size());

        if (computedHash != storedUser->passwordHash)
        {
            logger.log(QString("Failed login attempt for %1").arg(username));
            return QHttpServerResponse("Incorrect Username or Password", QHttpServerResponse::StatusCode::BadRequest);
        }

        QString authToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QDateTime expiresAt = QDateTime::currentDateTimeUtc().addDays(1);

        if (!database.storeSession(storedUser->id, authToken, expiresAt))
        {
            qWarning() << "Failed to store session:" << database.lastError();
            return QHttpServerResponse("Internal Server Error", QHttpServerResponse::StatusCode::InternalServerError);
        }
        QJsonObject response;
        // Send back the raw token to the client (client sends this back as a Bearer token in future requests)
        response["authToken"] = authToken;
        logger.log(QString("%1 logged in").arg(username));
        return QHttpServerResponse("application/json", QJsonDocument(response).toJson(QJsonDocument::Compact), QHttpServerResponse::StatusCode::Ok);
    });


    if(!server.bind(&tcpServer)){ // connect the TCP listener to the HTTP server
        qDebug() << "Failed to bind HTTP server";
    }

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    // Lets Main.qml use the name "logModel" to reach this C++ object.
    engine.rootContext()->setContextProperty("logModel", &logModel);
    engine.loadFromModule("homeserver", "Main");

    return QGuiApplication::exec();
}
