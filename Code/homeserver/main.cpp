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

    server.route("/events/", QHttpServerRequest::Method::Get, [&database](int id)
    {
        std::optional<QJsonObject> event = database.event(id);
        if(!event)
        {
            return QHttpServerResponse("Event not found", QHttpServerResponse::StatusCode::NotFound);
        }
        else
        {
            return QHttpServerResponse(*event, QHttpServerResponse::StatusCode::Ok);
        }
    });

    server.route("/events", QHttpServerRequest::Method::Post, [&database](const QHttpServerRequest &request)
    {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if(!doc.isObject())
        {
            return QHttpServerResponse("Invalid JSON payload", QHttpServerResponse::StatusCode::BadRequest);
        }
        std::optional<qint64> id = database.addEvent(doc.object());
        if(!id)
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
        //userList.push_back(DemoUser{username,hash,salt});
        std::optional<qint64> userId = database.createUser(username, passwordHash, salt);

        if (!userId)
        {
            qWarning() << "Failed to create user:" << database.lastError();
            return QHttpServerResponse("Failed to create user", QHttpServerResponse::StatusCode::InternalServerError);
        }

        return QHttpServerResponse("User Created", QHttpServerResponse::StatusCode::Created);
    });

    // temporary test route for login
    server.route("/login", QHttpServerRequest::Method::Post, [&userList](const QHttpServerRequest &request)
    {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        QJsonObject obj = doc.object();
        QString username = obj["username"].toString();
        QString password = obj["password"].toString();

        for(int i = 0; i < userList.size(); i++)
        {
            if (userList[i].username == username && userList[i].passwordHash == PasswordHasher::scryptHash(password,userList[i].salt))
            {
                QJsonObject response;
                response["authToken"] = "abc";
                return QHttpServerResponse("application/json", QJsonDocument(response).toJson(QJsonDocument::Compact), QHttpServerResponse::StatusCode::Ok);
            }
        }
        return QHttpServerResponse(("Incorrect Username or Password"), QHttpServerResponse::StatusCode::BadRequest);
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
