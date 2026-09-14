#include "MyTCPServer.h"

MyTCPServer::MyTCPServer(int port, QObject *parent)
    : QObject{parent}
{
    _server =  new QTcpServer(this);
    connect(_server, &QTcpServer::newConnection, this, &MyTCPServer::on_client_connecting);
    _isStarted = _server->listen(QHostAddress::Any, port); //default QHost address: every address can connect to it

    if(! _isStarted){
        qDebug() << "Server could not start";
    } else{
        qDebug() << "Server started...";
    }
}

//whenever client connects to a server
void MyTCPServer::on_client_connecting()
{
    qDebug() << "a client connected to server";
    auto socket = _server->nextPendingConnection(); // allows server to get pointer to socket that we use to interact with client
    connect(socket, &QTcpSocket::readyRead,this,&MyTCPServer::clientDataReady);
    connect(socket, &QTcpSocket::disconnected,this,&MyTCPServer::clientDisconnected);
    _socketsList.append(socket); //Add socket to list before moving on to next one
    socket->write("Welcome to this Server");
    emit newClientConnected();

}

void MyTCPServer::clientDisconnected()
{
    emit clientDisconnect(); //emit signal from TCP server clients
}

void MyTCPServer::clientDataReady()
{
    // read data from socket that sends data from the client
    auto socket = qobject_cast<QTcpSocket *>(sender()); // cast sender to instance of QTcp socket
    auto data = QString(socket->readAll()); // read data from socket

    emit dataReceived(data);
    foreach(auto s, _socketsList){
        if(s!=socket){
            s->write(data.toUtf8()); // pass data read by server from one client to all clients
        }
    }
}

bool MyTCPServer::isStarted() const
{
    return _isStarted;
}

void MyTCPServer::sendToAll(QString message)
{
    foreach(auto socket, _socketsList){
        socket->write(message.toUtf8());
    }
}


/*
 * Whenever a client connects to a server in QT, a signal named
 * new connection is ommited. We can create a new socket named newConnection
 * to handle those clients that want to connect our server
 */
