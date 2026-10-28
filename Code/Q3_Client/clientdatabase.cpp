// SQLite implementation of ClientDatabase. Public API contracts are in clientdatabase.h.
// Each operation clears its previous error; a missing record is not a SQL error.
#include "clientdatabase.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTimeZone>
#include <QUrl>
#include <QUuid>
#include <QVariant>

#include <algorithm>

// SQL expression for the current UTC time in milliseconds; works on any SQLite version.
static const QString NowMillis = "CAST((julianday('now') - 2440587.5) * 86400000 AS INTEGER)";

/**
 * @brief Convert stored UTC milliseconds to a QDateTime.
 * @param value Integer column value.
 * @return The UTC instant.
 */
static QDateTime fromMillis(const QVariant &value)
{
    return QDateTime::fromMSecsSinceEpoch(value.toLongLong(), QTimeZone::UTC);
}

/**
 * @brief Build a message from a row selected as id, contact_id, server_event_id, direction, body, sent_at, status.
 * @param query Query positioned on a message row.
 * @return The message record.
 */
static StoredMessage messageFromRow(const QSqlQuery &query)
{
    const QString status = query.value(6).toString();
    return StoredMessage{query.value(0).toLongLong(), query.value(1).toLongLong(),
        query.value(2).isNull() ? std::nullopt : std::optional<qint64>(query.value(2).toLongLong()),
        query.value(3).toString() == "in" ? MessageDirection::Incoming : MessageDirection::Outgoing,
        query.value(4).toString(), fromMillis(query.value(5)),
        status == "sent" ? MessageStatus::Sent : status == "failed" ? MessageStatus::Failed
            : status == "deleted" ? MessageStatus::Deleted : MessageStatus::Pending};
}

/**
 * @brief Allocate a unique name for this instance's Qt SQL connection.
 * @note The connection is established by open(). Use and destroy this instance on the opening thread.
 */
ClientDatabase::ClientDatabase()
    : m_connectionName(QUuid::createUuid().toString(QUuid::WithoutBraces))
{}

/**
 * @brief Close and unregister the connection owned by this instance.
 */
ClientDatabase::~ClientDatabase()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::database(m_connectionName).close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

/**
 * @brief Choose the database file for an account.
 *
 * @param username Account name; case-insensitive like the homeserver's usernames.
 * @return Path to <data dir>/<percent-encoded lowercase username>.sqlite.
 * @note The data directory is Q3_CLIENT_DATA_DIR if set, otherwise the platform's app data location.
 */
QString ClientDatabase::pathForUser(const QString &username)
{
    QString directory = qEnvironmentVariable("Q3_CLIENT_DATA_DIR");
    if (directory.isEmpty())
        directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    // One file per account keeps users on a shared machine apart. Lowercasing matches the
    // homeserver's case-insensitive usernames; percent-encoding keeps separators out of the name.
    const QString fileName = QString::fromLatin1(QUrl::toPercentEncoding(username.toLower())) + ".sqlite";
    return QDir(directory).filePath(fileName);
}

/**
 * @brief Name a message direction as stored in the database ("in" or "out").
 * @param direction Direction to convert.
 * @return The stored text form.
 */
QString ClientDatabase::toText(MessageDirection direction)
{
    return direction == MessageDirection::Incoming ? "in" : "out";
}

/**
 * @brief Name a message status as stored in the database ("pending", "sent", "failed", or "deleted").
 * @param status Status to convert.
 * @return The stored text form.
 */
QString ClientDatabase::toText(MessageStatus status)
{
    switch (status) {
    case MessageStatus::Pending: return "pending";
    case MessageStatus::Sent: return "sent";
    case MessageStatus::Failed: return "failed";
    case MessageStatus::Deleted: return "deleted";
    }
    return "pending";
}

/**
 * @brief Open or create an owner-only SQLite file and apply transactional schema migrations.
 *
 * @param filePath Database file, normally from pathForUser(). Missing directories are created owner-only.
 * @return true on success; false on file, connection, migration, or repeated-open failure.
 * @note Open once per instance; create a new instance to retry. Inspect lastError() on failure.
 */
bool ClientDatabase::open(const QString &filePath)
{
    m_lastError.clear();
    if (QSqlDatabase::contains(m_connectionName)) {
        m_lastError = "Store is already open; create another instance to reconnect";
        return false;
    }
    const QFileInfo file(filePath);
    // Restrict a directory this method creates; an existing directory keeps its permissions.
    if (!file.dir().exists()) {
        if (!QDir().mkpath(file.absolutePath())) {
            m_lastError = "Could not create data directory " + file.absolutePath();
            return false;
        }
        QFile::setPermissions(file.absolutePath(),
            QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    }
    // The file holds plaintext history, so create it owner-only before SQLite writes anything.
    // SQLite gives its journal files the same permissions as the database file.
    const QFileDevice::Permissions ownerOnly = QFileDevice::ReadOwner | QFileDevice::WriteOwner;
    if (!file.exists()) {
        QFile newFile(filePath);
        if (!newFile.open(QIODevice::WriteOnly, ownerOnly)) {
            m_lastError = newFile.errorString();
            return false;
        }
    }
    if (!QFile::setPermissions(filePath, ownerOnly)) {
        m_lastError = "Could not restrict permissions on " + filePath;
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    if (!db.isValid()) {
        m_lastError = "QSQLITE driver could not be loaded";
        return false;
    }
    db.setDatabaseName(filePath);
    if (!db.open()) {
        m_lastError = db.lastError().text();
        return false;
    }
    // SQLite leaves foreign keys off per connection unless enabled. secure_delete zeroes removed
    // content in the file. busy_timeout waits for another instance's lock instead of failing.
    QSqlQuery query(db);
    for (const char *pragma : {"PRAGMA foreign_keys = ON", "PRAGMA secure_delete = ON",
                               "PRAGMA busy_timeout = 5000"}) {
        if (!query.exec(pragma)) {
            m_lastError = query.lastError().text();
            return false;
        }
    }
    return migrate();
}

/**
 * @brief Apply supported schema upgrades in a single transaction.
 * @return true if the schema is ready; false if migration or version validation fails.
 * @note BEGIN IMMEDIATE serializes two instances opening the same file; failed upgrades roll back.
 */
bool ClientDatabase::migrate()
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    // Take the write lock up front so concurrent openers cannot both apply a migration.
    if (!query.exec("BEGIN IMMEDIATE")) {
        m_lastError = query.lastError().text();
        return false;
    }
    /**
     * @brief Roll back migration while preserving the original failure diagnostic.
     * @param message Original SQL or schema-version error.
     * @return false so callers can propagate the failure directly.
     */
    auto fail = [&](const QString &message) {
        m_lastError = message;
        query.finish();
        query.exec("ROLLBACK");
        return false;
    };
    if (!query.exec("CREATE TABLE IF NOT EXISTS schema_migrations (version INTEGER PRIMARY KEY, "
                    "applied_at INTEGER NOT NULL DEFAULT (" + NowMillis + "))"))
        return fail(query.lastError().text());
    if (!query.exec("SELECT COALESCE(MAX(version), 0) FROM schema_migrations") || !query.next())
        return fail(query.lastError().text());
    // Refuse files written by a newer client rather than guessing compatibility.
    const int version = query.value(0).toInt();
    query.finish();
    if (version > 1)
        return fail("Database schema is newer than this application supports");
    if (version == 0) {
        // Everything below, including the version marker, commits or rolls back together.
        // Never edit an applied version; add the next version's block below instead.
        const QStringList statements = {
            // NOCASE matches the homeserver's case-insensitive usernames (ASCII letters only).
            // identity_key is reserved for pinning a peer's key once key exchange exists.
            "CREATE TABLE contacts (id INTEGER PRIMARY KEY, "
                "username TEXT NOT NULL UNIQUE COLLATE NOCASE "
                    "CHECK (username = trim(username) AND length(username) > 0), "
                "identity_key BLOB, "
                "created_at INTEGER NOT NULL DEFAULT (" + NowMillis + "))",
            // AUTOINCREMENT never reuses IDs, so beforeId paging cursors stay valid.
            // A unique server_event_id makes re-synced events idempotent; NULLs do not conflict.
            "CREATE TABLE messages (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "contact_id INTEGER NOT NULL REFERENCES contacts(id) ON DELETE CASCADE, "
                "server_event_id INTEGER UNIQUE, "
                "direction TEXT NOT NULL CHECK (direction IN ('in', 'out')), "
                "body TEXT NOT NULL, "
                "sent_at INTEGER NOT NULL, "
                "created_at INTEGER NOT NULL DEFAULT (" + NowMillis + "), "
                "status TEXT NOT NULL CHECK (status IN ('pending', 'sent', 'failed', 'deleted')))",
            // Supports per-conversation paging by message ID.
            "CREATE INDEX messages_contact_id_idx ON messages(contact_id, id)",
            "CREATE TABLE settings (key TEXT PRIMARY KEY, value TEXT NOT NULL)",
            "INSERT INTO schema_migrations(version) VALUES (1)"
        };
        for (const QString &statement : statements)
            if (!query.exec(statement))
                return fail(query.lastError().text());
    }
    if (!query.exec("COMMIT"))
        return fail(query.lastError().text());
    return true;
}

/**
 * @brief Read the diagnostic from the latest database operation.
 * @return An error message, or an empty string after success or a missing lookup.
 */
QString ClientDatabase::lastError() const
{
    return m_lastError;
}

/**
 * @brief Store a contact, or find the existing one with the same case-insensitive username.
 *
 * @param username Nonempty username without surrounding spaces.
 * @return The contact ID, or std::nullopt on constraint or SQL failure.
 * @note The first spelling stored is kept.
 */
std::optional<qint64> ClientDatabase::addContact(const QString &username)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    // The NOCASE unique column turns a repeated or case-variant add into a no-op.
    query.prepare("INSERT INTO contacts(username) VALUES (?) ON CONFLICT(username) DO NOTHING");
    query.addBindValue(username);
    if (!query.exec()) { m_lastError = query.lastError().text(); return std::nullopt; }
    const std::optional<StoredContact> stored = contact(username);
    if (!stored) {
        if (m_lastError.isEmpty()) m_lastError = "Contact was not stored";
        return std::nullopt;
    }
    return stored->id;
}

/**
 * @brief Look up a contact by username.
 *
 * @param username Username to match case-insensitively (ASCII letters only).
 * @return The contact, or std::nullopt if missing or the query fails.
 * @note A missing contact leaves lastError() empty.
 */
std::optional<StoredContact> ClientDatabase::contact(const QString &username)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    // The column's NOCASE collation applies to this comparison.
    query.prepare("SELECT id, username, identity_key, created_at FROM contacts WHERE username = ?");
    query.addBindValue(username);
    if (!query.exec()) { m_lastError = query.lastError().text(); return std::nullopt; }
    if (!query.next()) return std::nullopt;
    return StoredContact{query.value(0).toLongLong(), query.value(1).toString(),
        query.value(2).toByteArray(), fromMillis(query.value(3))};
}

/**
 * @brief List all contacts ordered by username.
 * @return A list (possibly empty), or std::nullopt on SQL failure.
 */
std::optional<QList<StoredContact>> ClientDatabase::contacts()
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec("SELECT id, username, identity_key, created_at FROM contacts ORDER BY username")) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    QList<StoredContact> contacts;
    while (query.next()) contacts.append({query.value(0).toLongLong(), query.value(1).toString(),
        query.value(2).toByteArray(), fromMillis(query.value(3))});
    return contacts;
}

/**
 * @brief Store a message in a contact's conversation.
 *
 * @param contactId ID of an existing contact.
 * @param direction Whether the local user sent or received the message.
 * @param body Plaintext message body.
 * @param sentAt Valid send time, stored as UTC milliseconds.
 * @param status Initial delivery state, normally Pending for outgoing messages.
 * @param serverEventId Homeserver event ID if already known, such as for a synced incoming message.
 * @return The message ID, or std::nullopt on validation, constraint, or SQL failure.
 * @note A serverEventId that is already stored returns the existing message unchanged, so syncing is idempotent.
 */
std::optional<qint64> ClientDatabase::addMessage(qint64 contactId, MessageDirection direction,
    const QString &body, const QDateTime &sentAt, MessageStatus status,
    std::optional<qint64> serverEventId)
{
    m_lastError.clear();
    if (!sentAt.isValid()) { m_lastError = "A message requires a valid send time"; return std::nullopt; }
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    // A server event that is already stored (such as our own message coming back in a sync)
    // inserts nothing and returns no row; foreign-key and CHECK violations still fail.
    query.prepare("INSERT INTO messages(contact_id, server_event_id, direction, body, sent_at, status) "
                  "VALUES (?, ?, ?, ?, ?, ?) ON CONFLICT(server_event_id) DO NOTHING RETURNING id");
    query.addBindValue(contactId);
    query.addBindValue(serverEventId ? QVariant(*serverEventId) : QVariant(QMetaType::fromType<qint64>()));
    query.addBindValue(toText(direction));
    query.addBindValue(body);
    query.addBindValue(sentAt.toMSecsSinceEpoch());
    query.addBindValue(toText(status));
    if (!query.exec()) { m_lastError = query.lastError().text(); return std::nullopt; }
    if (query.next()) return query.value(0).toLongLong();
    if (!serverEventId) { m_lastError = "Message was not stored"; return std::nullopt; }

    query.finish();
    query.prepare("SELECT id FROM messages WHERE server_event_id = ?");
    query.addBindValue(*serverEventId);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text().isEmpty() ? "Message was not stored" : query.lastError().text();
        return std::nullopt;
    }
    return query.value(0).toLongLong();
}

/**
 * @brief Record a message's delivery result.
 *
 * @param messageId ID of the message to update.
 * @param status New delivery state.
 * @param serverEventId Homeserver event ID to record; std::nullopt keeps the stored value.
 * @return true if the message exists; false if it does not or on SQL failure.
 * @note A deleted message stays deleted, but still records a new serverEventId.
 */
bool ClientDatabase::updateMessageStatus(qint64 messageId, MessageStatus status,
                                         std::optional<qint64> serverEventId)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    // A late server reply must not turn a deleted tombstone back into a visible, empty message.
    query.prepare("UPDATE messages SET status = CASE WHEN status = 'deleted' THEN status ELSE ? END, "
                  "server_event_id = COALESCE(?, server_event_id) WHERE id = ?");
    query.addBindValue(toText(status));
    query.addBindValue(serverEventId ? QVariant(*serverEventId) : QVariant(QMetaType::fromType<qint64>()));
    query.addBindValue(messageId);
    if (!query.exec()) { m_lastError = query.lastError().text(); return false; }
    if (query.numRowsAffected() == 0) { m_lastError = "No message has that ID"; return false; }
    return true;
}

/**
 * @brief Retrieve one page of a conversation in ascending message-ID order.
 *
 * @param contactId ID of the contact whose conversation to read.
 * @param beforeId Exclusive upper bound on message ID for older pages; zero returns the newest page.
 * @param limit Maximum number of results, from 1 to 1000; defaults to 100.
 * @return A list (possibly empty), or std::nullopt on validation or SQL failure.
 * @note Messages are ordered by when they were stored locally, not by sentAt.
 */
std::optional<QList<StoredMessage>> ClientDatabase::messages(qint64 contactId, qint64 beforeId, int limit)
{
    m_lastError.clear();
    if (beforeId < 0 || limit < 1 || limit > 1000) {
        m_lastError = "Require a nonnegative cursor and limit between 1 and 1000";
        return std::nullopt;
    }
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    // Select the newest page below the cursor, then reverse it for display order.
    query.prepare("SELECT id, contact_id, server_event_id, direction, body, sent_at, status FROM messages "
                  "WHERE contact_id = ? AND (? = 0 OR id < ?) ORDER BY id DESC LIMIT ?");
    query.addBindValue(contactId);
    query.addBindValue(beforeId);
    query.addBindValue(beforeId);
    query.addBindValue(limit);
    if (!query.exec()) { m_lastError = query.lastError().text(); return std::nullopt; }
    QList<StoredMessage> messages;
    while (query.next()) messages.append(messageFromRow(query));
    std::reverse(messages.begin(), messages.end());
    return messages;
}

/**
 * @brief Erase a message body and mark it deleted.
 *
 * @param messageId ID of the message to delete.
 * @return true if the message exists; false if it does not or on SQL failure.
 * @note The row is kept as a tombstone so a later sync cannot restore the message.
 */
bool ClientDatabase::deleteMessage(qint64 messageId)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    // Keeping server_event_id lets addMessage() recognize the event if it is synced again.
    query.prepare("UPDATE messages SET body = '', status = 'deleted' WHERE id = ?");
    query.addBindValue(messageId);
    if (!query.exec()) { m_lastError = query.lastError().text(); return false; }
    if (query.numRowsAffected() == 0) { m_lastError = "No message has that ID"; return false; }
    return true;
}

/**
 * @brief Read a stored setting such as "homeserver_url" or "last_event_id".
 *
 * @param key Setting name.
 * @return The value, or std::nullopt if unset or the query fails.
 * @note A missing setting leaves lastError() empty.
 */
std::optional<QString> ClientDatabase::setting(const QString &key)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare("SELECT value FROM settings WHERE key = ?");
    query.addBindValue(key);
    if (!query.exec()) { m_lastError = query.lastError().text(); return std::nullopt; }
    if (!query.next()) return std::nullopt;
    return query.value(0).toString();
}

/**
 * @brief Create or replace a setting.
 *
 * @param key Setting name.
 * @param value New value.
 * @return true on success; false on SQL failure.
 */
bool ClientDatabase::setSetting(const QString &key, const QString &value)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare("INSERT INTO settings(key, value) VALUES (?, ?) "
                  "ON CONFLICT(key) DO UPDATE SET value = excluded.value");
    query.addBindValue(key);
    query.addBindValue(value);
    if (!query.exec()) { m_lastError = query.lastError().text(); return false; }
    return true;
}
