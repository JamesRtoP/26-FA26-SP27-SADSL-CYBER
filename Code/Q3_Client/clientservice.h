#ifndef CLIENTSERVICE_H
#define CLIENTSERVICE_H

#include <QString>
#include <QObject>
#include <QNetworkAccessManager> // sends HTTP requests
#include <QVariantList>
#include <memory>
#include "clientdatabase.h" // local message storage

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
    // stored messages with a recipient, oldest first, for QML to display
    Q_INVOKABLE QVariantList conversationHistory(const QString &recipient, int limit = 100);

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
    bool openDatabase(const QString &username);

    QNetworkAccessManager networkManager;
    QString authToken;
    QString currentUsername;
    // signed-in user's local database; opened at login, one file per account
    std::unique_ptr<ClientDatabase> database;
};

#endif // CLIENTSERVICE_H
