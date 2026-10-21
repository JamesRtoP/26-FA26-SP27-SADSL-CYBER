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
    QList<QJsonObject> events;

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

    server.route("/events/", QHttpServerRequest::Method::Get, [&events](int id)
    {
        if(id < 1 || events.size() < id)
        {
            return QHttpServerResponse("Event not found", QHttpServerResponse::StatusCode::NotFound);
        }
        else
        {
            return QHttpServerResponse(events[id-1], QHttpServerResponse::StatusCode::Ok);
        }
    });

    server.route("/events", QHttpServerRequest::Method::Post, [&events](const QHttpServerRequest &request)
    {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if(!doc.isObject())
        {
            return QHttpServerResponse("Invalid JSON payload", QHttpServerResponse::StatusCode::BadRequest);
        }
        QJsonObject newEvent = doc.object();
        events.push_back(newEvent);
        return QHttpServerResponse("Event Posted", QHttpServerResponse::StatusCode::Created);
    });

    // temporary test route for registration
    server.route("/register", QHttpServerRequest::Method::Post, [&userList](const QHttpServerRequest &request)
    {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        QJsonObject obj = doc.object();
        QString username = obj["username"].toString();

        for(int i = 0; i < userList.size(); i++)
        {
            if (userList[i].username == username)
            {
                return QHttpServerResponse("Username Taken", QHttpServerResponse::StatusCode::BadRequest);
            }
        }

        QString password = obj["password"].toString();
        std::vector<unsigned char> salt = PasswordHasher::salt();
        std::vector<unsigned char> hash = PasswordHasher::scryptHash(password, salt);
        userList.push_back(DemoUser{username,hash,salt});
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
