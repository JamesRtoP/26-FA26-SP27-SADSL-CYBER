#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _server = nullptr;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnStartServer_clicked()
{

    if(_server == nullptr){
        auto port = ui->spnServerPort->value();
        _server = new MyTCPServer(port);
        connect(_server, &MyTCPServer::newClientConnected, this, &MainWindow::newClientConnected);
        connect(_server, &MyTCPServer::dataReceived, this, &MainWindow::clientDataReceived);
        connect(_server, &MyTCPServer::clientDisconnect, this, &MainWindow::clientDisconnected);

    }
     QString state = _server->isStarted()? QStringLiteral("1") : QStringLiteral("0");
     ui->lblConnectionStatus->setProperty("state",state);
     ui->lblConnectionStatus->style()->polish(ui->lblConnectionStatus);



}

void MainWindow::newClientConnected()
{
    ui->lstConsole->addItem("New Client connected");
}


//passes message to method that goes through each socket in the socketList and relays message
void MainWindow::on_btnSendToAll_clicked()
{
    auto message = ui->lnMessage->text().trimmed();
    _server->sendToAll(message);
}

void MainWindow::clientDisconnected()
{
    ui->lstConsole->addItem("Client Disconnected");

}

void MainWindow::clientDataReceived(QString message)
{
    ui->lstConsole->addItem(message);

}

