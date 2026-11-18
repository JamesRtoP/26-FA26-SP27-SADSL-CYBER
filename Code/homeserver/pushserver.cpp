#include "pushserver.h"

#include "database.h"

#include <QDebug>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QWebSocket>

// A client that connects but never sends its token gets disconnected after this long,
// so idle or malicious connections can't pile up.
static constexpr int AuthTimeoutMs = 10000;

// Clients only ever send a small auth message, so refuse anything bigger than this.
static constexpr quint64 MaxMessageBytes = 4096;

PushServer::PushServer(Database &database, QObject *parent)
    : QObject(parent)
    , m_database(database)
    // NonSecureMode means plain ws:// with no encryption.
    // TODO before production: switch to SecureMode (wss://) so traffic is encrypted.
    , m_server("homeserver", QWebSocketServer::NonSecureMode)
{
    // Signals and slots: when m_server emits "newConnection" (a client connected),
    // Qt automatically calls our onNewConnection() function.
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
    // The open "phone line" to one specific client.
    QWebSocket *socket = m_server.nextPendingConnection();
    qDebug() << "WebSocket connected from" << socket->peerAddress().toString();

    socket->setMaxAllowedIncomingMessageSize(MaxMessageBytes);

    // When this socket receives text, call onTextMessage and tell it WHICH socket.
    connect(socket, &QWebSocket::textMessageReceived, this,
            [this, socket](const QString &text) { onTextMessage(socket, text); });

    // When this socket disconnects, call onDisconnected for cleanup.
    connect(socket, &QWebSocket::disconnected, this,
            [this, socket]() { onDisconnected(socket); });

    // If the client hasn't proven who it is after AuthTimeoutMs, hang up on it.
    // Passing "socket" as the second argument means the timer is cancelled
    // automatically if the socket is deleted first.
    QTimer::singleShot(AuthTimeoutMs, socket, [this, socket]() {
        if (!m_socketUsers.contains(socket)) {
            socket->close(QWebSocketProtocol::CloseCodePolicyViolated,
                          QStringLiteral("Authentication timeout"));
        }
    });
}

void PushServer::onTextMessage(QWebSocket *socket, const QString &text)
{
    // Once a client is authenticated it has nothing more to say, so ignore anything else.
    if (m_socketUsers.contains(socket)) {
        return;
    }

    // Parse the text as JSON. If it isn't valid JSON, we get an empty object back.
    const QJsonObject message = QJsonDocument::fromJson(text.toUtf8()).object();

    // The first message must be {"type":"auth", ...}. Anything else is a protocol violation.
    if (message.value("type").toString() != QLatin1String("auth")) {
        socket->close(QWebSocketProtocol::CloseCodePolicyViolated,
                      QStringLiteral("Expected auth message"));
        return;
    }

    // Ask the database: is this token a real, unexpired, unrevoked session?
    // If yes we get that session's user ID. If not we get nothing (std::nullopt).
    const QString token = message.value("authToken").toString();
    const std::optional<qint64> userId = m_database.sessionUser(token.toUtf8());

    if (!userId) {
        socket->sendTextMessage(QStringLiteral("{\"type\":\"error\",\"message\":\"Authentication failed\"}"));
        socket->close(QWebSocketProtocol::CloseCodePolicyViolated,
                      QStringLiteral("Authentication failed"));
        return;
    }

    // Success: remember who this socket belongs to, in both lookup tables.
    m_socketUsers.insert(socket, *userId);
    m_userSockets[*userId].insert(socket);
    qDebug() << "WebSocket authenticated as user" << *userId;

    socket->sendTextMessage(QStringLiteral("{\"type\":\"authenticated\"}"));
}

void PushServer::onDisconnected(QWebSocket *socket)
{
    qDebug() << "WebSocket disconnected";

    // Only authenticated sockets are in the tables, so look this one up first.
    const auto entry = m_socketUsers.find(socket);
    if (entry != m_socketUsers.end()) {
        const qint64 userId = entry.value();
        m_socketUsers.erase(entry);

        // Remove the socket from that user's set, and drop the user's entry
        // entirely once they have no sockets left.
        const auto sockets = m_userSockets.find(userId);
        if (sockets != m_userSockets.end()) {
            sockets->remove(socket);
            if (sockets->isEmpty()) {
                m_userSockets.erase(sockets);
            }
        }
    }

    // We own this socket, so free it when the client leaves.
    // deleteLater() deletes it safely once Qt is finished with it.
    socket->deleteLater();
}