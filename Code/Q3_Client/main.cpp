#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "clientservice.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    ClientService clientService;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    // tells QML the cpp object, clientService, can be used
    engine.rootContext()->setContextProperty("clientService", &clientService);
    engine.loadFromModule("Q3_Client", "Main");

    return QGuiApplication::exec();
}
