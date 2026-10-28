#ifndef CLIENTSERVICE_H
#define CLIENTSERVICE_H

#include <QString>
#include <QObject>
#include <QNetworkAccessManager>
#include <QVariantList>
#include <memory>

#include "clientdatabase.h"

class ClientService : public QObject
{
    Q_OBJECT

public:
    explicit ClientService(QObject *parent = nullptr);

    Q_INVOKABLE void registerUser(
        const QString &username,
        const QString &password);

    Q_INVOKABLE void loginUser(
        const QString &username,
        const QString &password);

    Q_INVOKABLE void startChat(
        const QString &recipient);

    Q_INVOKABLE void sendMessage(
        const QString &recipient,
        const QString &message);

    // Retrieve the user's event queue from the homeserver.
    Q_INVOKABLE void loadEvents();

    // Stored local messages with one recipient, oldest first.
    Q_INVOKABLE QVariantList conversationHistory(
        const QString &recipient,
        int limit = 100);

signals:
    void loginSucceeded(const QString &authToken);
    void loginFailed(const QString &message);

    void chatStarted();
    void chatFailed(const QString &message);

    void messageSent();
    void messageFailed(const QString &message);

    void eventsLoaded(const QVariantList &events);
    void eventsLoadFailed(const QString &message);

    void registrationSucceeded();
    void registrationFailed(const QString &message);

private:
    bool openDatabase(const QString &username);

    QNetworkAccessManager networkManager;
    QString authToken;
    QString currentUsername;

    // One local SQLite database per signed-in account.
    std::unique_ptr<ClientDatabase> database;
};

#endif // CLIENTSERVICE_H