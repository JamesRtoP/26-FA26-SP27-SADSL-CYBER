# Client local database

The client stores each signed-in account's contacts, message history, and settings in a local SQLite file. This is separate from the homeserver's PostgreSQL database: it holds what only the client may know (readable messages now, private encryption keys later), and it never leaves the device.

SQLite is built into Qt (`QSQLITE` driver), so no database installation or environment setup is needed.

## File location

One file per account, opened when login succeeds:

| Platform | Default path |
|---|---|
| macOS | `~/Library/Application Support/appQ3_Client/<username>.sqlite` |
| Windows | `%APPDATA%/appQ3_Client/<username>.sqlite` |
| Linux | `~/.local/share/appQ3_Client/<username>.sqlite` |

- `<username>` is lowercased (usernames are case-insensitive on the homeserver) and percent-encoded, so names containing `/` or `..` cannot escape the directory.
- Set `Q3_CLIENT_DATA_DIR` to use another directory, for example when testing two accounts or keeping test data apart.
- Separate files keep accounts on a shared machine apart, so Alice and Bob can be demonstrated on one computer.

## Security

- The file and any directory the client creates for it are **owner-only** (`0600` / `0700`).
- **The file is not encrypted yet.** Message bodies are stored as plaintext. Encryption at rest (SQLCipher, or column encryption with a password-derived key) is planned alongside the client cryptography work.
- `PRAGMA secure_delete` zeroes deleted content inside the database file. Deleted messages keep a tombstone row with an empty body, so a later sync cannot restore them.

## Schema (version 1)

- `contacts`: ID, username (unique, case-insensitive, no surrounding spaces), `identity_key` (reserved for pinning a peer's public identity key once key exchange exists), creation time.
- `messages`: ID (never reused), contact ID, `server_event_id` (unique when present; makes re-synced events idempotent), direction `in`/`out`, body, `sent_at`, `created_at`, status `pending`/`sent`/`failed`/`deleted`. Deleting a contact deletes its messages.
- `settings`: key/value pairs. Reserved keys are `homeserver_url` and `last_event_id` (the sync cursor for the homeserver inbox).
- `schema_migrations`: applied versions. Startup applies missing versions in one `BEGIN IMMEDIATE` transaction and rejects files written by a newer client. **Never edit an applied version; add a new one.** Key-store tables for PQXDH/SPQR will be version 2.

Times are stored as UTC milliseconds since the Unix epoch.

## C++ API (`clientdatabase.h`)

`ClientDatabase` follows the same pattern as the homeserver's `Database`: one connection per instance, `open()` once, use it on one thread, `lastError()` for diagnostics, and `std::optional`/`bool` results. A missing lookup returns an empty optional with an empty error.

- `pathForUser(username)`: file path for an account.
- `addContact(username)`: insert or find; returns the ID and keeps the first spelling. `contact(username)` and `contacts()` read contacts.
- `addMessage(contactId, direction, body, sentAt, status, serverEventId?)`: returns the message ID. A server event ID that is already stored returns the existing message.
- `updateMessageStatus(id, status, serverEventId?)`: records a delivery result. A deleted message stays deleted.
- `messages(contactId, beforeId = 0, limit = 100)`: newest page (or the page before `beforeId`) in ascending order. The limit must be 1–1000.
- `deleteMessage(id)`: erases the body and marks the message `deleted`.
- `setting(key)` / `setSetting(key, value)`.

## How `ClientService` uses it

- **Login:** opens the account's file. If storage cannot be opened, login fails with "Could not open local message storage".
- **Start Chat:** stores the recipient as a contact once the homeserver accepts the chat.
- **Send Message:** stores the message as `pending` before sending, then marks it `sent` (with the server's event ID if the response has `{"id": n}`) or `failed`. A message that cannot be saved locally is not sent.
- **QML:** `clientService.conversationHistory(recipient, limit)` returns `[{id, direction, body, sentAt, status}]`, oldest first.

The homeserver does not implement `/user/events` yet, so chats cannot start and sent messages are marked `failed` until that route exists.

## Inspect a database

```bash
sqlite3 ~/Library/Application\ Support/appQ3_Client/alice.sqlite
```
```sql
.tables
.schema messages
SELECT * FROM schema_migrations;
SELECT c.username, m.direction, m.body, m.status, datetime(m.sent_at / 1000, 'unixepoch') AS sent
FROM messages m JOIN contacts c ON c.id = m.contact_id ORDER BY m.id;
```
Close the client before editing a database by hand.

## Unit tests

```bash
cmake -S Code/Q3_Client -B <build dir> -DQ3_CLIENT_BUILD_TESTS=ON -DCMAKE_PREFIX_PATH=<Qt 6.11 kit>
cmake --build <build dir>
ctest --test-dir <build dir> --output-on-failure
```
In Qt Creator, add `-DQ3_CLIENT_BUILD_TESTS=ON` to the CMake configuration and run the `tst_clientdatabase` target. Each test uses a temporary file. The suite covers schema creation and versioning, owner-only permissions, path containment, contact uniqueness, message status, sync deduplication, paging, foreign keys, tombstones, and settings.
