#include "pushserver.h"

#include <QDebug>
#include <QHostAddress>
#include <QWebSocket>

PushServer::PushServer(QObject *parent)
    : QObject(parent)
    // The first argument is just a name for this server.
    // NonSecureMode means plain ws:// with no encryption.
    // TODO before production: switch to SecureMode (wss://) so traffic is encrypted.
    , m_server("homeserver", QWebSocketServer::NonSecureMode)
{
    // Signals and slots: when m_server emits "newConnection" (a client connected),
    // Qt automatically calls our onNewConnection() function.
    // We never write a loop that waits for clients.
    connect(&m_server, &QWebSocketServer::newConnection,
            this, &PushServer::onNewConnection);
}

bool PushServer::listen(quint16 port)
{
    // QHostAddress::Any means "accept connections on every network interface".
    return m_server.listen(QHostAddress::Any, port);
}

void PushServer::onNewConnection()
{
    // Get the connection that just came in. A QWebSocket is the open "phone line"
    // to one specific client. In step 2 we'll store these so we know who is on each line.
    QWebSocket *socket = m_server.nextPendingConnection();
    qDebug() << "WebSocket connected from" << socket->peerAddress().toString();

    // When this client disconnects (closes the app, loses Wi-Fi, etc.), log it.
    connect(socket, &QWebSocket::disconnected, this, []() {
        qDebug() << "WebSocket disconnected";
    });

    // We own this socket now, so we must free it when the client leaves.
    // deleteLater() deletes it safely once Qt is finished with it.
    // Without this line, every connection would leak memory.
    connect(socket, &QWebSocket::disconnected, socket, &QObject::deleteLater);
}