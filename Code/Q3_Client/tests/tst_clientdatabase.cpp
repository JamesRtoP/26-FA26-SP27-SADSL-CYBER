// Unit tests for ClientDatabase. Each test uses a throwaway SQLite file in a temporary directory.
#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include <memory>

#include "clientdatabase.h"

/**
 * @brief Run one SQL statement on a separate connection to inspect or tamper with a file directly.
 * @param path Database file.
 * @param sql Statement to execute.
 * @return The first column of the first row, or an invalid QVariant if there is none.
 */
static QVariant rawValue(const QString &path, const QString &sql)
{
    QVariant value;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "raw");
        db.setDatabaseName(path);
        if (db.open()) {
            QSqlQuery query(db);
            if (query.exec(sql) && query.next()) value = query.value(0);
        }
    }
    QSqlDatabase::removeDatabase("raw");
    return value;
}

class TestClientDatabase : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void openCreatesOwnerOnlyFileAndSchema();
    void reopenKeepsDataWithoutRemigrating();
    void secondOpenOnSameInstanceFails();
    void rejectsNewerSchema();
    void pathForUserIsCaseInsensitiveAndContained();
    void addContactIsIdempotentAndCaseInsensitive();
    void rejectsBlankOrPaddedUsernames();
    void missingLookupsLeaveErrorEmpty();
    void messageLifecycle();
    void duplicateServerEventReturnsExistingMessage();
    void messagesPageNewestFirstInAscendingOrder();
    void rejectsInvalidPaging();
    void foreignKeysRejectUnknownContact();
    void deleteKeepsTombstone();
    void settingsRoundTrip();

private:
    QString path() const { return m_dir->filePath("user.sqlite"); }
    std::unique_ptr<QTemporaryDir> m_dir;
};

void TestClientDatabase::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
}

void TestClientDatabase::openCreatesOwnerOnlyFileAndSchema()
{
    {
        ClientDatabase db;
        QVERIFY2(db.open(path()), qPrintable(db.lastError()));
        QVERIFY(db.lastError().isEmpty());
    }
#ifdef Q_OS_UNIX
    const QFileDevice::Permissions others = QFileDevice::ReadGroup | QFileDevice::WriteGroup
        | QFileDevice::ReadOther | QFileDevice::WriteOther;
    QCOMPARE(QFile::permissions(path()) & others, QFileDevice::Permissions());
#endif
    QCOMPARE(rawValue(path(), "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name IN "
                              "('contacts', 'messages', 'settings', 'schema_migrations')").toInt(), 4);
    QCOMPARE(rawValue(path(), "SELECT MAX(version) FROM schema_migrations").toInt(), 1);
}

void TestClientDatabase::reopenKeepsDataWithoutRemigrating()
{
    {
        ClientDatabase db;
        QVERIFY(db.open(path()));
        QVERIFY(db.addContact("Alice"));
    }
    ClientDatabase db;
    QVERIFY2(db.open(path()), qPrintable(db.lastError()));
    QVERIFY(db.contact("alice"));
    QCOMPARE(rawValue(path(), "SELECT COUNT(*) FROM schema_migrations").toInt(), 1);
}

void TestClientDatabase::secondOpenOnSameInstanceFails()
{
    ClientDatabase db;
    QVERIFY(db.open(path()));
    QVERIFY(!db.open(path()));
    QVERIFY(!db.lastError().isEmpty());
}

void TestClientDatabase::rejectsNewerSchema()
{
    {
        ClientDatabase db;
        QVERIFY(db.open(path()));
    }
    rawValue(path(), "INSERT INTO schema_migrations(version) VALUES (2)");
    ClientDatabase db;
    QVERIFY(!db.open(path()));
    QVERIFY(db.lastError().contains("newer"));
}

void TestClientDatabase::pathForUserIsCaseInsensitiveAndContained()
{
    qputenv("Q3_CLIENT_DATA_DIR", m_dir->path().toUtf8());
    QCOMPARE(ClientDatabase::pathForUser("Alice"), ClientDatabase::pathForUser("alice"));
    QCOMPARE(QFileInfo(ClientDatabase::pathForUser("Alice")).absolutePath(), QDir(m_dir->path()).absolutePath());
    // Separators and traversal must stay inside the data directory as a single file name.
    QCOMPARE(QFileInfo(ClientDatabase::pathForUser("../evil/x")).absolutePath(), QDir(m_dir->path()).absolutePath());
    QVERIFY(ClientDatabase::pathForUser("../evil/x") != ClientDatabase::pathForUser("evil"));
    qunsetenv("Q3_CLIENT_DATA_DIR");
}

void TestClientDatabase::addContactIsIdempotentAndCaseInsensitive()
{
    ClientDatabase db;
    QVERIFY(db.open(path()));
    const std::optional<qint64> first = db.addContact("Alice");
    QVERIFY2(first, qPrintable(db.lastError()));
    QCOMPARE(db.addContact("Alice"), first);
    QCOMPARE(db.addContact("ALICE"), first);
    QCOMPARE(db.contact("alice")->username, QString("Alice")); // first spelling kept
    QVERIFY(db.addContact("Bob"));
    const std::optional<QList<StoredContact>> all = db.contacts();
    QVERIFY(all);
    QCOMPARE(all->size(), 2);
    QCOMPARE(all->at(0).username, QString("Alice"));
    QVERIFY(all->at(0).identityKey.isEmpty());
    QVERIFY(all->at(0).createdAt.isValid());
}

void TestClientDatabase::rejectsBlankOrPaddedUsernames()
{
    ClientDatabase db;
    QVERIFY(db.open(path()));
    QVERIFY(!db.addContact(""));
    QVERIFY(!db.lastError().isEmpty());
    QVERIFY(!db.addContact(" alice"));
    QVERIFY(!db.lastError().isEmpty());
}

void TestClientDatabase::missingLookupsLeaveErrorEmpty()
{
    ClientDatabase db;
    QVERIFY(db.open(path()));
    QVERIFY(!db.contact("nobody"));
    QVERIFY(db.lastError().isEmpty());
    QVERIFY(!db.setting("homeserver_url"));
    QVERIFY(db.lastError().isEmpty());
}

void TestClientDatabase::messageLifecycle()
{
    ClientDatabase db;
    QVERIFY(db.open(path()));
    const qint64 bob = *db.addContact("Bob");
    const QDateTime sentAt = QDateTime::fromMSecsSinceEpoch(1793000000123, QTimeZone::UTC);
    const std::optional<qint64> id = db.addMessage(bob, MessageDirection::Outgoing, "hello", sentAt,
                                                   MessageStatus::Pending);
    QVERIFY2(id, qPrintable(db.lastError()));
    QVERIFY(db.updateMessageStatus(*id, MessageStatus::Sent, 42));

    const std::optional<QList<StoredMessage>> history = db.messages(bob);
    QVERIFY(history);
    QCOMPARE(history->size(), 1);
    const StoredMessage &message = history->first();
    QCOMPARE(message.id, *id);
    QCOMPARE(message.contactId, bob);
    QCOMPARE(message.serverEventId, std::optional<qint64>(42));
    QCOMPARE(message.direction, MessageDirection::Outgoing);
    QCOMPARE(message.body, QString("hello"));
    QCOMPARE(message.sentAt, sentAt);
    QCOMPARE(message.status, MessageStatus::Sent);

    // A status update without a server ID keeps the recorded one.
    QVERIFY(db.updateMessageStatus(*id, MessageStatus::Failed));
    QCOMPARE(db.messages(bob)->first().serverEventId, std::optional<qint64>(42));

    QVERIFY(!db.updateMessageStatus(999999, MessageStatus::Sent));
    QVERIFY(!db.lastError().isEmpty());
    QVERIFY(!db.addMessage(bob, MessageDirection::Outgoing, "x", QDateTime(), MessageStatus::Pending));
}

void TestClientDatabase::duplicateServerEventReturnsExistingMessage()
{
    ClientDatabase db;
    QVERIFY(db.open(path()));
    const qint64 bob = *db.addContact("Bob");
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const std::optional<qint64> first = db.addMessage(bob, MessageDirection::Incoming, "hi", now,
                                                      MessageStatus::Sent, 7);
    QVERIFY2(first, qPrintable(db.lastError()));
    QCOMPARE(db.addMessage(bob, MessageDirection::Incoming, "hi again", now, MessageStatus::Sent, 7), first);
    QCOMPARE(db.messages(bob)->size(), 1);
    QCOMPARE(db.messages(bob)->first().body, QString("hi"));
    // Messages without a server ID never conflict with each other.
    QVERIFY(db.addMessage(bob, MessageDirection::Outgoing, "a", now, MessageStatus::Pending));
    QVERIFY(db.addMessage(bob, MessageDirection::Outgoing, "b", now, MessageStatus::Pending));
    QCOMPARE(db.messages(bob)->size(), 3);
}

void TestClientDatabase::messagesPageNewestFirstInAscendingOrder()
{
    ClientDatabase db;
    QVERIFY(db.open(path()));
    const qint64 bob = *db.addContact("Bob");
    const qint64 carol = *db.addContact("Carol");
    QList<qint64> ids;
    for (int i = 0; i < 5; ++i)
        ids.append(*db.addMessage(bob, MessageDirection::Outgoing, QString::number(i),
                                  QDateTime::currentDateTimeUtc(), MessageStatus::Sent));
    QVERIFY(db.addMessage(carol, MessageDirection::Outgoing, "other", QDateTime::currentDateTimeUtc(),
                          MessageStatus::Sent));

    const QList<StoredMessage> newest = *db.messages(bob, 0, 2);
    QCOMPARE(newest.size(), 2);
    QCOMPARE(newest.at(0).id, ids.at(3));
    QCOMPARE(newest.at(1).id, ids.at(4));
    const QList<StoredMessage> older = *db.messages(bob, newest.at(0).id, 2);
    QCOMPARE(older.size(), 2);
    QCOMPARE(older.at(0).id, ids.at(1));
    QCOMPARE(older.at(1).id, ids.at(2));
    QCOMPARE(db.messages(bob)->size(), 5); // Carol's message is not in Bob's conversation
}

void TestClientDatabase::rejectsInvalidPaging()
{
    ClientDatabase db;
    QVERIFY(db.open(path()));
    QVERIFY(!db.messages(1, 0, 0));
    QVERIFY(!db.lastError().isEmpty());
    QVERIFY(!db.messages(1, 0, 1001));
    QVERIFY(!db.messages(1, -1, 10));
    QVERIFY(db.messages(1, 0, 1000));
    QVERIFY(db.messages(1, 0, 1000)->isEmpty());
}

void TestClientDatabase::foreignKeysRejectUnknownContact()
{
    ClientDatabase db;
    QVERIFY(db.open(path()));
    QVERIFY(!db.addMessage(12345, MessageDirection::Outgoing, "x", QDateTime::currentDateTimeUtc(),
                           MessageStatus::Pending));
    QVERIFY(!db.lastError().isEmpty());
}

void TestClientDatabase::deleteKeepsTombstone()
{
    ClientDatabase db;
    QVERIFY(db.open(path()));
    const qint64 bob = *db.addContact("Bob");
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const qint64 id = *db.addMessage(bob, MessageDirection::Incoming, "secret", now, MessageStatus::Sent, 5);
    QVERIFY(db.deleteMessage(id));

    StoredMessage message = db.messages(bob)->first();
    QCOMPARE(message.status, MessageStatus::Deleted);
    QVERIFY(message.body.isEmpty());
    // Neither a late delivery result nor a re-synced copy brings the message back.
    QVERIFY(db.updateMessageStatus(id, MessageStatus::Sent));
    QCOMPARE(db.addMessage(bob, MessageDirection::Incoming, "secret", now, MessageStatus::Sent, 5), id);
    message = db.messages(bob)->first();
    QCOMPARE(message.status, MessageStatus::Deleted);
    QVERIFY(message.body.isEmpty());

    QVERIFY(!db.deleteMessage(999999));
    QVERIFY(!db.lastError().isEmpty());
}

void TestClientDatabase::settingsRoundTrip()
{
    ClientDatabase db;
    QVERIFY(db.open(path()));
    QVERIFY(db.setSetting("homeserver_url", "http://127.0.0.1:8080"));
    QCOMPARE(db.setting("homeserver_url"), std::optional<QString>("http://127.0.0.1:8080"));
    QVERIFY(db.setSetting("homeserver_url", "http://example.test"));
    QCOMPARE(db.setting("homeserver_url"), std::optional<QString>("http://example.test"));
}

QTEST_GUILESS_MAIN(TestClientDatabase)
#include "tst_clientdatabase.moc"
