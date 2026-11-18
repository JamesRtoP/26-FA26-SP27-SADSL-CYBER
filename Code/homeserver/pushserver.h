#ifndef PUSHSERVER_H
#define PUSHSERVER_H

#include <QObject>
#include <QWebSocketServer>

/*
 * PushServer lets the homeserver talk to clients first.
 *
 * Normal HTTP works like mail: the client asks, the server answers, and the
 * conversation ends. That means the server can't tell a client "you have a new
 * message" on its own.
 *
 * A WebSocket is more like a phone call. The client connects once and the
 * connection stays open, so the server can send data down it whenever it wants.
 *
 * Right now this class only accepts connections. Later steps will add
 * authentication and sending messages.
 */
class PushServer : public QObject
{
    // Required for any class that uses Qt signals and slots
    Q_OBJECT

public:
    explicit PushServer(QObject *parent = nullptr);

    // Start accepting WebSocket connections on the given port.
    // Returns true if it worked, false if it failed (for example, the port is taken).
    bool listen(quint16 port);

private:
    // Runs every time a new client connects
    void onNewConnection();

    // Does the actual listening. Same idea as the QTcpServer in main.cpp,
    // but it speaks the WebSocket protocol.
    QWebSocketServer m_server;
};

#endif // PUSHSERVER_H