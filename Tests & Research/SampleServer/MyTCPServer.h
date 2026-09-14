#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QObject>
#include <QTcpServer> // needed to run QTCP server
#include <QTcpSocket> // Each client that connects to server is managed by a QTcp socket


class MyTCPServer : public QObject
{
    Q_OBJECT
public:
    explicit MyTCPServer(int port, QObject *parent = nullptr);
    bool isStarted() const;
    void sendToAll(QString message);

signals:
    void newClientConnected();
    void clientDisconnect();
    void dataReceived(QString message);
private slots:
    void on_client_connecting();
    void clientDisconnected();
    void clientDataReady();
private:
    QTcpServer *_server;
    QList<QTcpSocket *> _socketsList; // Since we are going to connect different clients to this server and need to keep track of all of them
    bool _isStarted;
};
#endif // MYTCPSERVER_H
