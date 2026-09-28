#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtWebEngineQuick>

int main(int argc, char *argv[])
{
    QtWebEngineQuick::initialize();
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("Qt_Kibana_Integration", "Main");

    return QGuiApplication::exec();
}
