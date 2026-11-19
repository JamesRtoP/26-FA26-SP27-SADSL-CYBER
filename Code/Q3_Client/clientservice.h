#ifndef CLIENTSERVICE_H
#define CLIENTSERVICE_H

#include <QTimer>
#include <QString>
#include <QObject>
#include <QNetworkAccessManager>
#include <QVariantList>
#include <memory>
#include <QWebSocket>

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

    void messageReceived(const QString &sender, const QString &body,
                            const QDateTime &sentAt);

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

    // The open WebSocket line to the homeserver
    QWebSocket liveSocket;

    // Reconnect state for live socket.
    QTimer reconnectTimer; // fires when it's time to try connecting again
    int reconnectDelayMs = 1000; // current wait, doubles after each failed attempt
    bool wantLive = false; // true while a user is logged in



    // Opens the WebSocket connection to the homeerver's push port
    void connectLive();

    // Handles one text message pushed by the homeserver on the live socket
    void handleLiveMessage(const QString &text);

    // Applies one event from the homeserver. Stores it locally and tells the UI.
    // Called when events are pushed over the live socket
    void processEvent(qint64 id, const QJsonObject &body);

    // Schedules another live-connection attempt after a growing delay
    void scheduleReconnect();
};

#endif // CLIENTSERVICE_H