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

#include "eventstore.h"

QString healthCheck(){
    return "OK";
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QHttpServer server; // handles the HTTP requests
    QTcpServer tcpServer; // listens on the port
    EventStore events; // events are stored in PostgreSQL

    if(!events.open())
    {
        qCritical() << "Failed to connect to the database:" << events.lastError();
        return -1;
    }

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
        std::optional<QJsonObject> event = events.event(id);
        if(!event)
        {
            return QHttpServerResponse("Event not found", QHttpServerResponse::StatusCode::NotFound);
        }
        else
        {
            return QHttpServerResponse(*event, QHttpServerResponse::StatusCode::Ok);
        }
    });

    server.route("/events", QHttpServerRequest::Method::Post, [&events](const QHttpServerRequest &request)
    {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if(!doc.isObject())
        {
            return QHttpServerResponse("Invalid JSON payload", QHttpServerResponse::StatusCode::BadRequest);
        }
        std::optional<qint64> id = events.addEvent(doc.object());
        if(!id)
        {
            qWarning() << "Failed to store event:" << events.lastError();
            return QHttpServerResponse("Failed to store event", QHttpServerResponse::StatusCode::InternalServerError);
        }
        return QHttpServerResponse("Event Posted", QHttpServerResponse::StatusCode::Created);
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
