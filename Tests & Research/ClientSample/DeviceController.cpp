#include "DeviceController.h"

DeviceController::DeviceController(QObject *parent)
    : QObject{parent}
{
    //if the signal connected from the socket was ommited, omit the connected signal and device controller automatically
    connect(&_socket, &QTcpSocket::connected, this, &DeviceController::connected);
    connect(&_socket, &QTcpSocket::disconnected, this, &DeviceController::disconnected);
    connect(&_socket, &QTcpSocket::errorOccurred, this, &DeviceController::errorOccurred);
    connect(&_socket, &QTcpSocket::stateChanged, this, &DeviceController::socket_stateChanged);
    connect(&_socket, &QTcpSocket::readyRead, this, &DeviceController::socket_readyRead);

}
void DeviceController::connectToDevice(QString ip, int port)
{
    if(_socket.isOpen()){
        if (ip == _ip && port == _port){ //nothing happens if true
            return; //because IP is already connected to this port
        }
        _socket.close(); //since device is open and IP ports differ
    }
    _ip = ip;
    _port = port;
    _socket.connectToHost(_ip, _port); //connect IP & port
}

void DeviceController::disconnect()
{
    _socket.close();
}

QAbstractSocket::SocketState DeviceController::state()
{
    return _socket.state();
}

bool DeviceController::isConnected()
{
    return _socket.state() == QAbstractSocket::ConnectedState;
}

void DeviceController::send(QString message)
{
    _socket.write(message.toUtf8()); // convert to Qbyte array to easily send to server
}

void DeviceController::socket_stateChanged(QAbstractSocket::SocketState state)
{
    if(state == QAbstractSocket::UnconnectedState){
        _socket.close();
    }
    emit stateChanged(state); //inform other layers what happened
}

void DeviceController::socket_readyRead()
{
    auto data = _socket.readAll(); // returns a Q-byte array of all data read from the server
    emit dataReady(data);
}
