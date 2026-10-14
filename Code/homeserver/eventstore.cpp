#include "eventstore.h"

#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

static const QString connectionName = QStringLiteral("homeserver");

// Reads a setting from the environment, falling back to a default
static QString setting(const char *name, const QString &fallback)
{
    QString value = qEnvironmentVariable(name);
    return value.isEmpty() ? fallback : value;
}

EventStore::~EventStore()
{
    if(QSqlDatabase::contains(connectionName))
    {
        QSqlDatabase::database(connectionName).close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

bool EventStore::open()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", connectionName);
    // Invalid when the QPSQL plugin exists but libpq could not be loaded
    if(!db.isValid())
    {
        m_lastError = "QPSQL driver could not be loaded, check that libpq is on DYLD_FALLBACK_LIBRARY_PATH";
        return false;
    }
    db.setHostName(setting("HOMESERVER_DB_HOST", "localhost"));
    db.setPort(setting("HOMESERVER_DB_PORT", "5432").toInt());
    db.setDatabaseName(setting("HOMESERVER_DB_NAME", "homeserver"));
    db.setUserName(setting("HOMESERVER_DB_USER", "homeserver"));
    db.setPassword(qEnvironmentVariable("HOMESERVER_DB_PASSWORD"));

    if(!db.open())
    {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    if(!query.exec("CREATE TABLE IF NOT EXISTS events ("
                   "id BIGSERIAL PRIMARY KEY, "
                   "body JSONB NOT NULL, "
                   "created_at TIMESTAMPTZ NOT NULL DEFAULT now())"))
    {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

QString EventStore::lastError() const
{
    return m_lastError;
}

std::optional<qint64> EventStore::addEvent(const QJsonObject &event)
{
    QSqlQuery query(QSqlDatabase::database(connectionName));
    query.prepare("INSERT INTO events (body) VALUES (CAST(? AS JSONB)) RETURNING id");
    query.addBindValue(QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)));

    if(!query.exec() || !query.next())
    {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    return query.value(0).toLongLong();
}

std::optional<QJsonObject> EventStore::event(qint64 id)
{
    QSqlQuery query(QSqlDatabase::database(connectionName));
    query.prepare("SELECT body::text FROM events WHERE id = ?");
    query.addBindValue(id);

    if(!query.exec())
    {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    if(!query.next())
    {
        return std::nullopt;
    }
    return QJsonDocument::fromJson(query.value(0).toString().toUtf8()).object();
}
