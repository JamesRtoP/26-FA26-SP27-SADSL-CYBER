#ifndef CLIENTDATABASE_H
#define CLIENTDATABASE_H

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>
#include <optional>

// A conversation peer. Conversations are one-to-one, so a contact is also its conversation.
struct StoredContact {
    qint64 id;
    QString username;
    QByteArray identityKey; // Empty until key exchange exists to pin the peer's identity key.
    QDateTime createdAt;
};

// Which side of the conversation wrote a message.
enum class MessageDirection { Incoming, Outgoing };

// Delivery state of a message. Deleted messages remain as tombstones with an empty body.
enum class MessageStatus { Pending, Sent, Failed, Deleted };

// A message as stored on this device. The body is plaintext here; never send this record to a server.
struct StoredMessage {
    qint64 id;
    qint64 contactId;
    std::optional<qint64> serverEventId; // Homeserver event ID, once the server has accepted the message.
    MessageDirection direction;
    QString body;
    QDateTime sentAt;
    MessageStatus status;
};

// Local SQLite storage for one signed-in account: contacts, message history, and settings; no HTTP logic.
// The file is owner-only but not yet encrypted at rest. Call open() before querying, and
// use/destroy the instance on that same thread. Queries bind caller values as parameters.
class ClientDatabase
{
public:
    /**
     * @brief Allocate a unique name for this instance's Qt SQL connection.
     * @note The connection is established by open(). Use and destroy this instance on the opening thread.
     */
    ClientDatabase();
    /**
     * @brief Close and unregister the connection owned by this instance.
     */
    ~ClientDatabase();
    /**
     * @brief Disable copying because a connection must have a single owner.
     * @param other ClientDatabase instance that cannot be copied.
     */
    ClientDatabase(const ClientDatabase &other) = delete;
    /**
     * @brief Disable assignment to preserve unique connection ownership.
     * @param other ClientDatabase instance that cannot be assigned.
     */
    ClientDatabase &operator=(const ClientDatabase &other) = delete;

    /**
     * @brief Choose the database file for an account.
     *
     * @param username Account name; case-insensitive like the homeserver's usernames.
     * @return Path to <data dir>/<percent-encoded lowercase username>.sqlite.
     * @note The data directory is Q3_CLIENT_DATA_DIR if set, otherwise the platform's app data location.
     */
    static QString pathForUser(const QString &username);
    /**
     * @brief Name a message direction as stored in the database ("in" or "out").
     * @param direction Direction to convert.
     * @return The stored text form.
     */
    static QString toText(MessageDirection direction);
    /**
     * @brief Name a message status as stored in the database ("pending", "sent", "failed", or "deleted").
     * @param status Status to convert.
     * @return The stored text form.
     */
    static QString toText(MessageStatus status);

    /**
     * @brief Open or create an owner-only SQLite file and apply transactional schema migrations.
     *
     * @param filePath Database file, normally from pathForUser(). Missing directories are created owner-only.
     * @return true on success; false on file, connection, migration, or repeated-open failure.
     * @note Open once per instance; create a new instance to retry. Inspect lastError() on failure.
     */
    bool open(const QString &filePath);
    /**
     * @brief Read the diagnostic from the latest database operation.
     * @return An error message, or an empty string after success or a missing lookup.
     */
    QString lastError() const;

    /**
     * @brief Store a contact, or find the existing one with the same case-insensitive username.
     *
     * @param username Nonempty username without surrounding spaces.
     * @return The contact ID, or std::nullopt on constraint or SQL failure.
     * @note The first spelling stored is kept.
     */
    std::optional<qint64> addContact(const QString &username);
    /**
     * @brief Look up a contact by username.
     *
     * @param username Username to match case-insensitively (ASCII letters only).
     * @return The contact, or std::nullopt if missing or the query fails.
     * @note A missing contact leaves lastError() empty.
     */
    std::optional<StoredContact> contact(const QString &username);
    /**
     * @brief List all contacts ordered by username.
     * @return A list (possibly empty), or std::nullopt on SQL failure.
     */
    std::optional<QList<StoredContact>> contacts();

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
    std::optional<qint64> addMessage(qint64 contactId, MessageDirection direction,
        const QString &body, const QDateTime &sentAt, MessageStatus status,
        std::optional<qint64> serverEventId = std::nullopt);
    /**
     * @brief Record a message's delivery result.
     *
     * @param messageId ID of the message to update.
     * @param status New delivery state.
     * @param serverEventId Homeserver event ID to record; std::nullopt keeps the stored value.
     * @return true if the message exists; false if it does not or on SQL failure.
     * @note A deleted message stays deleted, but still records a new serverEventId.
     */
    bool updateMessageStatus(qint64 messageId, MessageStatus status,
                             std::optional<qint64> serverEventId = std::nullopt);
    /**
     * @brief Retrieve one page of a conversation in ascending message-ID order.
     *
     * @param contactId ID of the contact whose conversation to read.
     * @param beforeId Exclusive upper bound on message ID for older pages; zero returns the newest page.
     * @param limit Maximum number of results, from 1 to 1000; defaults to 100.
     * @return A list (possibly empty), or std::nullopt on validation or SQL failure.
     * @note Messages are ordered by when they were stored locally, not by sentAt.
     */
    std::optional<QList<StoredMessage>> messages(qint64 contactId, qint64 beforeId = 0, int limit = 100);
    /**
     * @brief Erase a message body and mark it deleted.
     *
     * @param messageId ID of the message to delete.
     * @return true if the message exists; false if it does not or on SQL failure.
     * @note The row is kept as a tombstone so a later sync cannot restore the message.
     */
    bool deleteMessage(qint64 messageId);

    /**
     * @brief Read a stored setting such as "homeserver_url" or "last_event_id".
     *
     * @param key Setting name.
     * @return The value, or std::nullopt if unset or the query fails.
     * @note A missing setting leaves lastError() empty.
     */
    std::optional<QString> setting(const QString &key);
    /**
     * @brief Create or replace a setting.
     *
     * @param key Setting name.
     * @param value New value.
     * @return true on success; false on SQL failure.
     */
    bool setSetting(const QString &key, const QString &value);
private:
    /**
     * @brief Apply supported schema upgrades in a single transaction.
     * @return true if the schema is ready; false if migration or version validation fails.
     * @note BEGIN IMMEDIATE serializes two instances opening the same file; failed upgrades roll back.
     */
    bool migrate();
    QString m_connectionName; // Key in Qt's process-wide connection registry.
    QString m_lastError; // Latest operation's diagnostic.
};
#endif
