#ifndef CLIENTSERVICE_H
#define CLIENTSERVICE_H

#include <QString>
#include <QObject>
#include <QNetworkAccessManager> // sends HTTP requests

class ClientService : public QObject
{
    Q_OBJECT

public:
    explicit ClientService(QObject *parent = nullptr);

    // Q_INVOKABLE lets cpp function be called from QML
    Q_INVOKABLE void registerUser(const QString &username, const QString &password);
    Q_INVOKABLE void loginUser(const QString &username, const QString &password);

signals:

private:
    QNetworkAccessManager networkManager;
};

#endif // CLIENTSERVICE_H
