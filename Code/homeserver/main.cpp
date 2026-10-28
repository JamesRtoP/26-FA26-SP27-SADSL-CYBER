/*
 * Simplle homeserver to accept and respond to GET requests
 */

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

QString healthCheck(){
    return "OK";
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QHttpServer server; // handles the HTTP requests
    QTcpServer tcpServer; // listens on the port
    Database database; // events are stored in PostgreSQL

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

    // When someone requests /health, run healthCheck()
    server.route("/health", healthCheck);

    server.route("/user/events", QHttpServerRequest::Method::Get, [&database](const QHttpServerRequest &request)
    {
        QUrlQuery query(request.url());
        QString username = query.queryItemValue("username");
        QString authToken = query.queryItemValue("authToken");
        std::optional<qint64> userId = database.sessionUser(authToken.toUtf8());
        std::optional<StoredUser> storedUser = database.user(username);

        if(!userId.has_value() || !storedUser.has_value() || userId != storedUser->id)
        {
            return QHttpServerResponse("Authentication Mismatch", QHttpServerResponse::StatusCode::BadRequest);
        }

        std::optional<QList<StoredEvent>> newEvents = database.eventsForUser(userId.value());
        QJsonArray jsonArray;
        for (StoredEvent &ev: *newEvents)
        {
            QJsonObject eventObj;
            eventObj["id"]= ev.id;
            eventObj["body"]= ev.body;
            jsonArray.append(eventObj);
        }

        return QHttpServerResponse(jsonArray, QHttpServerResponse::StatusCode::Ok);
    });

    server.route("/user/events", QHttpServerRequest::Method::Post, [&database](const QHttpServerRequest &request)
    {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if(!doc.isObject())
        {
            return QHttpServerResponse("Invalid JSON payload", QHttpServerResponse::StatusCode::BadRequest);
        }
        QJsonObject event = doc.object();

        QList<qint64> userIDs;
        userIDs.append(database.user(event.value("sender").toString())->id);
        std::optional<StoredUser> storedUser = database.user(event.value("recipient").toString());
        if(!storedUser.has_value())
        {
            return QHttpServerResponse("Recipient Does Not Exist", QHttpServerResponse::StatusCode::InternalServerError);
        }
        userIDs.append(storedUser->id);
        std::optional<qint64> id = database.addEvent(event);

        std::optional<qint64> id2 = database.addEventForUsers(event,userIDs);
        if(!id || !id2)
        {
            qWarning() << "Failed to store event:" << database.lastError();
            return QHttpServerResponse("Failed to store event", QHttpServerResponse::StatusCode::InternalServerError);
        }
        return QHttpServerResponse(QJsonObject{{"id", *id}}, QHttpServerResponse::StatusCode::Created);
    });

    // temporary test route for registration
    server.route("/register", QHttpServerRequest::Method::Post, [&database](const QHttpServerRequest &request)
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

        return QHttpServerResponse("User Created", QHttpServerResponse::StatusCode::Created);
    });

    server.route("/login", QHttpServerRequest::Method::Post, [&database](const QHttpServerRequest &request)
    {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        QJsonObject obj = doc.object();
        QString username = obj["username"].toString();
        QString password = obj["password"].toString();

        std::optional<StoredUser> storedUser = database.user(username);
        if (!storedUser)
        {
            return QHttpServerResponse("Incorrect Username or Password", QHttpServerResponse::StatusCode::BadRequest);
        }

        std::vector<unsigned char> saltVec(storedUser->salt.begin(), storedUser->salt.end());
        std::vector<unsigned char> computedHashVec = PasswordHasher::scryptHash(password, saltVec);
        QByteArray computedHash((const char*)computedHashVec.data(), computedHashVec.size());

        if (computedHash != storedUser->passwordHash)
        {
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
    engine.loadFromModule("homeserver", "Main");

    return QGuiApplication::exec();
}
