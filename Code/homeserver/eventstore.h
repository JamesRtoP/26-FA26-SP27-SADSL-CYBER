/*
 * Stores homeserver events in a PostgreSQL database through Qt SQL
 */

#ifndef EVENTSTORE_H
#define EVENTSTORE_H

#include <QJsonObject>
#include <QString>
#include <optional>

class EventStore
{
public:
    ~EventStore();

    // Connects using the HOMESERVER_DB_* environment variables and creates
    // the events table if it does not exist yet
    bool open();
    QString lastError() const;

    // Returns the id of the new event, or nothing if the insert failed
    std::optional<qint64> addEvent(const QJsonObject &event);
    // Returns the event with this id, or nothing if there is no such event
    std::optional<QJsonObject> event(qint64 id);

private:
    QString m_lastError;
};

#endif // EVENTSTORE_H
