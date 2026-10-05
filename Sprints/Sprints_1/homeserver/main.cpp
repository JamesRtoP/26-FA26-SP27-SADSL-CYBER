/*
 * Simplle homeserver to accept and respond to GET requests
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QHttpServer>
#include <QTcpServer>
#include <QHostAddress>
#include <QDebug>

QString healthCheck(){
    return "OK";
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QHttpServer server; // handles the HTTP requests
    QTcpServer tcpServer; // listens on the port
    bool listening = tcpServer.listen(QHostAddress:: Any, 8080); // listen on any interface
    if (!listening){
        qDebug() << "Server failed to start:" << tcpServer.errorString();
    }
    else{
        qDebug() << "Listening on port" << tcpServer.serverPort();
    }

    // When someone requests /health, run healthCheck()
    server.route("/health", healthCheck);

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
