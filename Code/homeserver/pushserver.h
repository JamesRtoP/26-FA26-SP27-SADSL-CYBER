#ifndef PUSHSERVER_H
#define PUSHSERVER_H

#include <QHash>             // lookup tables
#include <QObject>
#include <QSet>              // a user can have several sockets, with no duplicates
#include <QWebSocketServer>

class Database;
class QWebSocket;

/*
 * PushServer lets the homeserver talk to clients first.
 * (HTTP is like mail, WebSocket is like a phone call)
 *
 * How a client uses it:
 *   1. The client connects.
 *   2. The client's first message must be {"type":"auth","authToken":"..."}.
 *   3. If the token is valid, the server replies {"type":"authenticated"} and
 *      remembers which user is on this connection.
 */
class PushServer : public QObject
{
    Q_OBJECT

public:
    // The server needs the database to check whether a session token is valid.
    explicit PushServer(Database &database, QObject *parent = nullptr);   // new: takes database

    bool listen(quint16 port);

private:
    void onNewConnection();
    void onTextMessage(QWebSocket *socket, const QString &text);   // Client sends text
    void onDisconnected(QWebSocket *socket);                       // Client leaves

    Database &m_database;
    QWebSocketServer m_server;

    // Table 1: "which sockets belong to user 7?" Used when pushing messages.
    // A user can be logged in on several devices, so each user maps to a SET of sockets.
    QHash<qint64, QSet<QWebSocket *>> m_userSockets;

    // Table 2: Used for cleanup when a socket disconnects.
    // Only sockets that have proven who they are appear here.
    QHash<QWebSocket *, qint64> m_socketUsers;
};

#endif // PUSHSERVER_H