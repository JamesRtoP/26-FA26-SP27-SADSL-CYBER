# Homeserver database

The homeserver stores accounts, login sessions, and events in PostgreSQL through Qt SQL's QPSQL driver. `/register` and `/login` use this database. `/events` and `/events/<id>` store and read events but do not check authentication or ownership, so do not use them as evidence of private event access. `/user/events` (authenticated inbox delivery) is not implemented yet.

For setup see the README's Homeserver Database Setup. For step-by-step testing see [TESTING.md](../../TESTING.md) and the demonstration guide [TEST.md](TEST.md).

## Schema

- `users`: numeric ID, username, password hash (`BYTEA`), salt (`BYTEA`), creation time. Usernames must be nonempty with no surrounding ASCII spaces, and are unique under PostgreSQL `lower()` using the database's locale. The supplied spelling is preserved. Unicode normalization is not performed.
- `user_sessions`: user ID, unique `token` (`TEXT`, exactly 36 characters: the UUID returned by `/login`), creation/expiration/revocation timestamps. Multiple sessions per user are supported. Expired or revoked sessions never resolve through `sessionUser()`. Revocation is idempotent.
- `events`: ID, JSONB body, creation time. The server treats encrypted event payloads as opaque JSON; database storage does not encrypt or decrypt them.
- `user_events`: unique `(user_id, event_id)` inbox membership. An event can be in several inboxes. Foreign keys reject nonexistent users/events. Deleting a user deletes their sessions and inbox memberships but preserves event bodies. Deleting an event deletes its inbox memberships.
- `homeserver_schema_migrations`: applied migration versions. Version 1 adopts an existing events table and creates the other tables and indexes in a single transaction. Startup serializes migrations with a PostgreSQL transaction advisory lock. Schema versions newer than the code supports are rejected.

The database role must own the existing events table and be allowed to create tables/indexes in the active schema. Back up a real database before upgrading it.

### Databases created by older builds

Earlier builds created version 1 with a different layout: `users` also had `password_algorithm` and `password_parameters` columns, and `user_sessions` had a 32-byte `token_hash` instead of `token`. Such a database still reports version 1, so startup changes nothing, but registration then fails with HTTP 500 and sessions cannot be stored.

To check, run `\d user_sessions` in psql. If it shows `token_hash`, the database uses the old layout. For test databases, create a new database. A database with data worth keeping needs a version 2 migration that converts the old layout; none exists yet. **From now on, applied migrations must never be edited. Schema changes go in a new version.**

## Storage API

`Database` owns a unique Qt SQL connection. Open it once, use it on that same thread, and let its destructor close the connection. Separate instances use separate connections. Connection settings come from `HOMESERVER_DB_HOST`, `HOMESERVER_DB_PORT`, `HOMESERVER_DB_NAME`, `HOMESERVER_DB_USER`, and `HOMESERVER_DB_PASSWORD`.

| Method | Used by | Behavior |
|---|---|---|
| `createUser(username, passwordHash, salt)` | `/register` | Stores precomputed credentials and returns the new ID. The database checks for nonempty values, not cryptographic strength. `/register` hashes passwords with scrypt (`PasswordHasher`) and a random 16-byte salt. |
| `user(username)` | `/register`, `/login` | Case-insensitive lookup returning credentials for internal authentication. Never serialize this structure into an HTTP response. |
| `storeSession(userId, token, expiresAt)` | `/login` | Stores a 36-character token with a future expiry (`/login` uses 24 hours). |
| `sessionUser(token)` | not yet | Returns a user ID only for a nonrevoked, unexpired session. |
| `revokeSession(token)` | not yet | Revokes one session; unknown or already-revoked tokens succeed. |
| `assignEvent(userId, eventId)` | not yet | Adds inbox membership; repeating an assignment succeeds without duplication. |
| `addEventForUsers(body, userIds)` | not yet | Inserts one event and all memberships atomically. An empty recipient list fails, and any invalid recipient rolls back the whole insert. |
| `eventsForUser(userId, afterId, limit)` | not yet | Returns `{id, body}` records in increasing event-ID order. The limit defaults to 100 and must be 1–1000. An empty inbox or a nonexistent positive user ID returns an empty list. `afterId` is for pagination, not a reliable sync cursor; reliable sync needs a separate delivery-sequence design. |
| `addEvent(body)`, `event(id)` | `/events`, `/events/<id>` | Legacy event storage. Neither checks ownership. |

All methods clear `lastError()` on entry. Missing lookups return an empty optional with an empty error; SQL and validation failures set the error. SQL errors are for internal logs, not public responses.

### Known gaps

- **Session tokens are stored as plain text.** Anyone who can read `user_sessions` can use a live session. Storing a SHA-256 digest of each token (the original design) would prevent this.
- **The session methods disagree on the token type.** `storeSession()` takes `QString`, while `sessionUser()` and `revokeSession()` take `QByteArray`. Make them consistent before the first route uses lookup or revocation.
- **KDF settings are not stored per user.** The scrypt parameters are fixed in code, so changing them later would invalidate existing password hashes.

## Testing

- **Demonstration:** [TEST.md](TEST.md) walks through restart persistence and runs [database-demo.sql](database-demo.sql), which checks schema version, credential storage, case-insensitive username uniqueness, inbox isolation, shared events, duplicate assignments, atomic event insertion, and session expiry/revocation. It runs in a rolled-back transaction and only against `homeserver_demo_test`.
- **Full application:** [TESTING.md](../../TESTING.md) covers the curl checks for `/register` and `/login` and queries for inspecting stored users and sessions.
- An earlier C++ integration suite passed against local PostgreSQL before it was removed; there is currently no automated homeserver test target.

## Troubleshooting

- **QPSQL driver could not be loaded:** select a Qt 6 kit that includes QPSQL. On Apple Silicon, install an arm64 libpq (`brew install libpq`) and set `DYLD_FALLBACK_LIBRARY_PATH=/opt/homebrew/opt/libpq/lib` in the process actually running the program (Terminal or Qt Creator).
- **Connection refused:** verify the server host/port and that the PostgreSQL service is running.
- **Password authentication failed:** use the `homeserver` database-role password, not the macOS password or application-user credentials.
- **Permission denied for schema/table:** the database role needs ownership/DDL privileges for migrations.
- **Relation already exists during migration:** inspect any manually created conflicting tables. Migration 1 rolls back rather than overwriting unknown tables; do not delete real data to bypass this error.
- **`/register` returns 500 / `null value in column "password_algorithm"` in the log:** the database was created by an older build (see above).
- **`/user/events` returns 404:** not implemented yet.
