// PostgreSQL implementation of Database. Public API contracts are in database.h.
// Each operation clears its previous error; a missing record is not a SQL error.
#include "database.h"

#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QUuid>

/**
 * @brief Allocate a unique name for this instance's Qt SQL connection.
 * @note The connection is established by open(). Use and destroy this instance on the opening thread.
 */
Database::Database()
    : m_connectionName(QUuid::createUuid().toString(QUuid::WithoutBraces))
{}

/**
 * @brief Read a nonempty environment setting or use its default.
 * @param name Environment variable name.
 * @param fallback Value used when the variable is unset or empty.
 * @return The environment value or fallback.
 */
static QString setting(const char *name, const QString &fallback)
{
    QString value = qEnvironmentVariable(name);
    return value.isEmpty() ? fallback : value;
}

/**
 * @brief Close and unregister the connection owned by this instance.
 */
Database::~Database()
{
    if(QSqlDatabase::contains(m_connectionName))
    {
        QSqlDatabase::database(m_connectionName).close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

/**
 * @brief Connect to PostgreSQL and apply transactional schema migrations.
 * @return true on success; false on connection, migration, or repeated-open failure.
 * @note Reads HOMESERVER_DB_* settings. Open once per instance; create a new instance to retry. Inspect lastError() on failure.
 */
bool Database::open()
{
    m_lastError.clear();
    if (QSqlDatabase::contains(m_connectionName)) {
        m_lastError = "Store is already open; create another instance to reconnect";
        return false;
    }
    // QPSQL delegates the PostgreSQL protocol to libpq.
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", m_connectionName);
    // Invalid when the QPSQL plugin exists but libpq could not be loaded
    if(!db.isValid())
    {
        m_lastError = "QPSQL driver could not be loaded, check that libpq is on DYLD_FALLBACK_LIBRARY_PATH";
        return false;
    }
    // Keep deployment settings outside source; a password has no built-in default.
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

    return migrate();
}

/**
 * @brief Apply supported schema upgrades in a single transaction.
 * @return true if the schema is ready; false if migration or version validation fails.
 * @note Serializes concurrent startups with an advisory lock. Preserves existing events without assigning owners and rolls back failed upgrades.
 */
bool Database::migrate()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction()) {
        m_lastError = db.lastError().text();
        return false;
    }
    /**
     * @brief Roll back migration while preserving the original failure diagnostic.
     * @param message Original SQL or schema-version error.
     * @return false so callers can propagate the failure directly.
     */
    auto fail = [&](const QString &message) {
        m_lastError = message;
        db.rollback();
        return false;
    };
    QSqlQuery query(db);
    // Serialize schema changes across simultaneous server startups.
    if (!query.exec("SELECT pg_advisory_xact_lock(4212026)"))
        return fail(query.lastError().text());
    if (!query.exec("CREATE TABLE IF NOT EXISTS homeserver_schema_migrations ("
                    "version INTEGER PRIMARY KEY, applied_at TIMESTAMPTZ NOT NULL DEFAULT now())"))
        return fail(query.lastError().text());
    if (!query.exec("SELECT COALESCE(MAX(version), 0) FROM homeserver_schema_migrations") || !query.next())
        return fail(query.lastError().text());
    // Refuse schemas written by a newer application rather than guessing compatibility.
    const int version = query.value(0).toInt();
    if (version > 1)
        return fail("Database schema is newer than this application supports");
    if (version == 0) {
        // Adopt the original events table without rewriting IDs, bodies, or ownership.
        // Everything below, including the version marker, commits or rolls back together.
        const QStringList statements = {
            "CREATE TABLE IF NOT EXISTS events (id BIGSERIAL PRIMARY KEY, body JSONB NOT NULL, "
                "created_at TIMESTAMPTZ NOT NULL DEFAULT now())",
            // Store binary credentials plus enough KDF metadata for future verification.
            "CREATE TABLE users (id BIGSERIAL PRIMARY KEY, "
                "username TEXT NOT NULL CHECK (username = btrim(username) AND length(username) > 0), "
                "password_hash BYTEA NOT NULL CHECK (octet_length(password_hash) > 0), "
                "salt BYTEA NOT NULL CHECK (octet_length(salt) > 0), "
                "password_algorithm TEXT NOT NULL CHECK (length(btrim(password_algorithm)) > 0), "
                "password_parameters JSONB NOT NULL CHECK (jsonb_typeof(password_parameters) = 'object'), "
                "created_at TIMESTAMPTZ NOT NULL DEFAULT now())",
            // Preserve display spelling while preventing case-only duplicate accounts.
            "CREATE UNIQUE INDEX users_username_unique ON users (lower(username))",
            // Independent sessions support multiple devices and individual revocation.
            // User deletion also removes sessions; token uniqueness prevents ambiguity.
            "CREATE TABLE user_sessions (id BIGSERIAL PRIMARY KEY, "
                "user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE, "
                "token_hash BYTEA NOT NULL UNIQUE CHECK (octet_length(token_hash) = 32), "
                "created_at TIMESTAMPTZ NOT NULL DEFAULT now(), "
                "expires_at TIMESTAMPTZ NOT NULL CHECK (expires_at > created_at), revoked_at TIMESTAMPTZ)",
            "CREATE INDEX user_sessions_user_id_idx ON user_sessions(user_id)",
            // Many-to-many inbox membership avoids duplicating shared event bodies.
            // The primary key supports user-scoped ID pagination and deduplication.
            "CREATE TABLE user_events (user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE, "
                "event_id BIGINT NOT NULL REFERENCES events(id) ON DELETE CASCADE, "
                "PRIMARY KEY (user_id, event_id))",
            // Index the reverse direction for event deletion/foreign-key maintenance.
            "CREATE INDEX user_events_event_id_idx ON user_events(event_id)",
            "INSERT INTO homeserver_schema_migrations(version) VALUES (1)"
        };
        for (const QString &statement : statements)
            if (!query.exec(statement))
                return fail(query.lastError().text());
    }
    // Release the current result before committing the schema transaction.
    query.finish();
    if (!db.commit())
        return fail(db.lastError().text());
    return true;
}

/**
 * @brief Read the diagnostic from the latest database operation.
 * @return An error message, or an empty string after success or a missing lookup.
 * @note SQL diagnostics are for internal use and must not be exposed in API responses.
 */
QString Database::lastError() const
{
    return m_lastError;
}

/**
 * @brief Insert an event without assigning it to a user inbox.
 *
 * @param event Opaque JSON event payload to store.
 * @return The generated event ID, or std::nullopt on SQL failure.
 * @note Does not perform authorization. Prefer addEventForUsers() when recipients are known.
 */
std::optional<qint64> Database::addEvent(const QJsonObject &event)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    // PostgreSQL parses the bound JSON text; RETURNING retrieves the generated ID.
    query.prepare("INSERT INTO events (body) VALUES (CAST(? AS JSONB)) RETURNING id");
    query.addBindValue(QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)));

    if(!query.exec() || !query.next())
    {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    return query.value(0).toLongLong();
}

/**
 * @brief Fetch an event by its database ID without checking ownership.
 *
 * @param id ID of the event to retrieve.
 * @return The event body, or std::nullopt if missing or the query fails.
 * @note Callers must authorize access. A missing record leaves lastError() empty.
 */
std::optional<QJsonObject> Database::event(qint64 id)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
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

/**
 * @brief Store a user and their precomputed password credentials.
 *
 * @param username Nonempty username without surrounding spaces; uniqueness is case-insensitive.
 * @param passwordHash Nonempty binary password KDF output, never a plaintext password.
 * @param salt Nonempty per-user salt generated by the caller.
 * @param algorithm Nonempty name identifying the password KDF.
 * @param parameters JSON object containing the KDF settings needed for verification.
 * @return The generated user ID, or std::nullopt on constraint or SQL failure.
 * @note The caller is responsible for hashing, salt generation, and cryptographic strength validation.
 */
std::optional<qint64> Database::createUser(const QString &username,
    const QByteArray &passwordHash, const QByteArray &salt,
    const QString &algorithm, const QJsonObject &parameters)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare("INSERT INTO users(username, password_hash, salt, password_algorithm, password_parameters) "
                  "VALUES (?, ?, ?, ?, CAST(? AS JSONB)) RETURNING id");
    query.addBindValue(username);
    query.addBindValue(passwordHash);
    query.addBindValue(salt);
    query.addBindValue(algorithm);
    query.addBindValue(QString::fromUtf8(QJsonDocument(parameters).toJson(QJsonDocument::Compact)));
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    return query.value(0).toLongLong();
}

/**
 * @brief Look up an internal authentication record by username.
 *
 * @param username Username to match case-insensitively.
 * @return The stored credentials, or std::nullopt if missing or the query fails.
 * @note A missing user leaves lastError() empty. Never serialize this credential record into an API response.
 */
std::optional<StoredUser> Database::user(const QString &username)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    // Match the same lower() expression used by the unique username index.
    query.prepare("SELECT id, username, password_hash, salt, password_algorithm, password_parameters::text "
                  "FROM users WHERE lower(username) = lower(?)");
    query.addBindValue(username);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    if (!query.next()) return std::nullopt;
    return StoredUser{query.value(0).toLongLong(), query.value(1).toString(),
        query.value(2).toByteArray(), query.value(3).toByteArray(), query.value(4).toString(),
        QJsonDocument::fromJson(query.value(5).toString().toUtf8()).object()};
}

/**
 * @brief Store an independently expiring authentication session.
 *
 * @param userId ID of an existing user.
 * @param tokenHash Binary 32-byte SHA-256 digest of a high-entropy bearer token, not raw or hexadecimal text.
 * @param expiresAt Valid future expiration instant, normalized to UTC when stored.
 * @return true on insertion; false on validation, constraint, or SQL failure.
 * @note Token generation and hashing belong to the caller. Multiple sessions per user are supported.
 */
bool Database::storeSession(qint64 userId, const QByteArray &tokenHash, const QDateTime &expiresAt)
{
    m_lastError.clear();
    if (tokenHash.size() != 32 || !expiresAt.isValid() || expiresAt <= QDateTime::currentDateTimeUtc()) {
        m_lastError = "A session requires a 32-byte token digest and a future expiration";
        return false;
    }
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare("INSERT INTO user_sessions(user_id, token_hash, expires_at) VALUES (?, ?, ?)");
    query.addBindValue(userId);
    query.addBindValue(tokenHash);
    // Normalize the supplied instant; TIMESTAMPTZ preserves timezone-independent expiry.
    query.addBindValue(expiresAt.toUTC());
    if (!query.exec()) { m_lastError = query.lastError().text(); return false; }
    return true;
}

/**
 * @brief Resolve a token digest to the user of an active session.
 *
 * @param tokenHash Binary SHA-256 digest of the presented bearer token.
 * @return The user ID, or std::nullopt for an unknown, expired, revoked, or failed lookup.
 * @note Uses the database clock for expiry. Inactive or missing sessions leave lastError() empty.
 */
std::optional<qint64> Database::sessionUser(const QByteArray &tokenHash)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    // Enforce expiry in SQL using the database clock, even if cleanup has not run.
    query.prepare("SELECT user_id FROM user_sessions WHERE token_hash = ? "
                  "AND revoked_at IS NULL AND expires_at > now()");
    query.addBindValue(tokenHash);
    if (!query.exec()) { m_lastError = query.lastError().text(); return std::nullopt; }
    if (!query.next()) return std::nullopt;
    return query.value(0).toLongLong();
}

/**
 * @brief Idempotently revoke one authentication session.
 *
 * @param tokenHash Binary SHA-256 digest identifying the session.
 * @return true even if already revoked or unknown; false on SQL failure.
 * @note Preserves the first revocation timestamp and leaves other sessions active.
 */
bool Database::revokeSession(const QByteArray &tokenHash)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    // Preserve the first revocation timestamp when logout is requested repeatedly.
    query.prepare("UPDATE user_sessions SET revoked_at = COALESCE(revoked_at, now()) WHERE token_hash = ?");
    query.addBindValue(tokenHash);
    if (!query.exec()) { m_lastError = query.lastError().text(); return false; }
    return true; // Revocation is idempotent, including an unknown token.
}

/**
 * @brief Add an existing event to an existing user's inbox.
 *
 * @param userId ID of the recipient user.
 * @param eventId ID of the event to assign.
 * @return true for a new or duplicate assignment; false on constraint or SQL failure.
 * @note Foreign keys reject nonexistent records. The caller must authorize the assignment.
 */
bool Database::assignEvent(qint64 userId, qint64 eventId)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    // A duplicate membership is harmless, but foreign-key violations still fail.
    query.prepare("INSERT INTO user_events(user_id, event_id) VALUES (?, ?) ON CONFLICT DO NOTHING");
    query.addBindValue(userId);
    query.addBindValue(eventId);
    if (!query.exec()) { m_lastError = query.lastError().text(); return false; }
    return true;
}

/**
 * @brief Atomically insert an event and all its inbox assignments.
 *
 * @param body Opaque JSON event payload to store.
 * @param userIds Nonempty list of existing recipient IDs; duplicate IDs are allowed.
 * @return The generated event ID, or std::nullopt if any insertion or commit fails.
 * @note Any failure rolls back both the event and its assignments. Uses its own transaction on this connection.
 */
std::optional<qint64> Database::addEventForUsers(const QJsonObject &body, const QList<qint64> &userIds)
{
    m_lastError.clear();
    if (userIds.isEmpty()) { m_lastError = "At least one event recipient is required"; return std::nullopt; }
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction()) { m_lastError = db.lastError().text(); return std::nullopt; }
    // Both helpers use this same connection, so their writes join this transaction.
    // Keep the original helper error when rolling back a failed insertion/assignment.
    const auto id = addEvent(body);
    if (!id) { db.rollback(); return std::nullopt; }
    for (qint64 userId : userIds) {
        if (!assignEvent(userId, *id)) { db.rollback(); return std::nullopt; }
    }
    if (!db.commit()) { m_lastError = db.lastError().text(); db.rollback(); return std::nullopt; }
    return id;
}

/**
 * @brief Retrieve one user's inbox in ascending event-ID order.
 *
 * @param userId Positive ID selected by the authenticated caller.
 * @param afterId Nonnegative exclusive lower bound on event ID; zero starts at the beginning.
 * @param limit Maximum number of results, from 1 to 1000; defaults to 100.
 * @return A list (possibly empty), or std::nullopt on validation or SQL failure.
 * @note ID filtering supports pagination, not reliable delivery synchronization under concurrent commits or late assignments. A nonexistent positive user ID returns an empty list.
 */
std::optional<QList<StoredEvent>> Database::eventsForUser(qint64 userId, qint64 afterId, int limit)
{
    m_lastError.clear();
    if (userId <= 0 || afterId < 0 || limit < 1 || limit > 1000) {
        m_lastError = "Require a positive user ID, nonnegative cursor, and limit between 1 and 1000";
        return std::nullopt;
    }
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    // Filter membership before returning bodies; unrelated users' events never enter
    // the result. The authenticated user ID must be supplied by the future API layer.
    query.prepare("SELECT e.id, e.body::text FROM user_events ue JOIN events e ON e.id = ue.event_id "
                  "WHERE ue.user_id = ? AND ue.event_id > ? ORDER BY ue.event_id LIMIT ?");
    query.addBindValue(userId);
    query.addBindValue(afterId);
    query.addBindValue(limit);
    if (!query.exec()) { m_lastError = query.lastError().text(); return std::nullopt; }
    QList<StoredEvent> events;
    while (query.next()) events.append({query.value(0).toLongLong(),
        QJsonDocument::fromJson(query.value(1).toString().toUtf8()).object()});
    return events;
}
