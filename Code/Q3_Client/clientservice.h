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
    Q_INVOKABLE void startChat(const QString &recipient);
    Q_INVOKABLE void sendMessage(const QString &recipient, const QString &message);

signals:
    // will be used to pass the auth token throughout the client code
    void loginSucceeded(const QString &authToken);
    void loginFailed(const QString &message);

    void chatStarted();
    void chatFailed(const QString &message);

    void messageSent();
    void messageFailed(const QString &message);

    void registrationSucceeded();
    void registrationFailed(const QString &message);

private:
    QNetworkAccessManager networkManager;
    QString authToken;
    QString currentUsername;
};

#endif // CLIENTSERVICE_H
