# Homeserver database and testing

This change adds PostgreSQL storage and C++ database methods only. There are no registration, login, or `/user/events` endpoints. Existing HTTP event routes remain unscoped and do not enforce authentication. Do not use those routes as evidence of private event access.

## Schema

- `users`: numeric ID, username, password hash (`BYTEA`), salt (`BYTEA`), password algorithm, algorithm parameters (`JSONB`), creation time. Usernames must be nonempty with no surrounding ASCII spaces, and are unique under PostgreSQL `lower()` using the database's locale. Preserve the supplied spelling. Unicode normalization is not performed.
- `user_sessions`: user ID, unique 32-byte SHA-256 token digest, creation/expiration/revocation timestamps. Multiple sessions per user are supported. Expired/revoked sessions never resolve through `sessionUser()`. Revocation is idempotent.
- `events`: existing ID, JSONB body, creation time. The server treats encrypted event payloads as opaque JSON; database storage does not encrypt or decrypt them.
- `user_events`: unique `(user_id, event_id)` inbox membership. An event can be in several inboxes. Foreign keys reject nonexistent users/events. Deleting a user deletes their sessions and inbox memberships, but preserves event bodies. Deleting an event deletes its inbox memberships.
- `homeserver_schema_migrations`: applied migration versions. Version 1 adopts the old events table and creates the new tables/indexes in a single transaction. Existing events remain unassigned. Startup serializes migrations with a PostgreSQL transaction advisory lock. Future schema versions are rejected by older code.

The database role must own the existing events table and be allowed to create tables/indexes in the active schema. Existing IDs and JSON bodies are preserved. Back up a real database before upgrading it.

## Storage API

`Database` owns a unique Qt SQL connection. Open it once, use it on that same thread, and let its destructor close the connection. Separate instances use separate connections.

- `createUser(username, passwordHash, salt, algorithm, parameters)` stores precomputed credentials and returns the new ID. The database checks nonempty values, not cryptographic strength. The future authentication layer must choose a password KDF, generate salts, validate parameters, and verify passwords.
- `user(username)` returns credential metadata for internal authentication use. Do not serialize this structure into an HTTP response.
- `storeSession(userId, tokenHash, expiresAt)` accepts a **binary 32-byte digest**, not a 64-character hexadecimal string or a bearer token. The caller must generate a high-entropy random token and hash it with SHA-256. Hashing, token issuance, and token transport are outside this change.
- `sessionUser(tokenHash)` returns a user ID only for a nonrevoked, unexpired session.
- `revokeSession(tokenHash)` revokes one session.
- `assignEvent(userId, eventId)` adds inbox membership; repeated assignment succeeds without duplication.
- `addEventForUsers(body, userIds)` inserts one event and all memberships atomically. Empty recipient lists fail. Any invalid recipient rolls back the entire insert.
- `eventsForUser(userId, afterId, limit)` returns `{id, body}` records in increasing event-ID order. The limit defaults to 100 and must be 1–1000. An empty inbox/nonexistent positive user ID returns an empty list. `afterId` filters IDs; it is not a durable synchronization cursor when concurrent transactions or later assignments are possible. A reliable sync protocol needs a separate delivery sequence/design.
- Legacy `addEvent()` and `event()` are retained for existing routes; neither checks ownership.

All methods clear `lastError()` on entry. Missing lookups return an empty optional with an empty error; SQL/validation failures set the error. SQL errors are for internal diagnostics, not public responses. Failed revocation of an unknown token is treated as successful idempotent revocation.

## Database validation

The integration suite passed against local PostgreSQL before its source and CMake target were removed at the user's request. It verified schema migration and rollback, credential storage, username uniqueness, session expiry/revocation, inbox isolation, shared events, pagination, and atomic event assignment. Use the manual SQL walkthrough below to inspect the database yourself.

## Inspect the schema and behavior manually

To create visible sample data for SQL inspection, use a separate demo database:

1. As the PostgreSQL administrator, create another database:

   ```bash
   /Library/PostgreSQL/18/bin/psql -h localhost -U postgres -d postgres -W
   ```

   ```sql
   CREATE DATABASE homeserver_demo_test OWNER homeserver;
   \q
   ```

2. In Qt Creator's **Run Environment**, set `HOMESERVER_DB_NAME=homeserver_demo_test`, `HOMESERVER_DB_USER=homeserver`, `HOMESERVER_DB_PASSWORD` to your role password, and the host/port variables if needed. On your Mac, keep `DYLD_FALLBACK_LIBRARY_PATH=/opt/homebrew/opt/libpq/lib`. Build and run the homeserver once. Successful startup applies the migration. Stop it before continuing.

3. Connect to the demo database:

   ```bash
   /Library/PostgreSQL/18/bin/psql -h localhost -U homeserver -d homeserver_demo_test -W
   ```

   Inspect the tables:

   ```sql
   \dt
   \d users
   \d user_sessions
   \d user_events
   SELECT * FROM homeserver_schema_migrations;
   ```

   Expect the five tables listed in the schema section and migration version `1`.

4. Insert dummy credentials. These strings are **test fixtures**, not real password hashes; there is no login endpoint to use them with:

   ```sql
   INSERT INTO users(username, password_hash, salt, password_algorithm, password_parameters)
   VALUES ('Alice', decode('aabbcc', 'hex'), decode('112233', 'hex'), 'test-only', '{"cost":1}')
   RETURNING id AS alice_id \gset

   INSERT INTO users(username, password_hash, salt, password_algorithm, password_parameters)
   VALUES ('Bob', decode('ddeeff', 'hex'), decode('445566', 'hex'), 'test-only', '{}')
   RETURNING id AS bob_id \gset

   SELECT id, username, encode(password_hash, 'hex') AS hash,
          encode(salt, 'hex') AS salt, password_algorithm, password_parameters
   FROM users ORDER BY id;
   ```

   Expect two rows with separate hashes and salts. `\gset` saves returned IDs in psql variables; continue in the same psql session.

5. Check username uniqueness:

   ```sql
   INSERT INTO users(username, password_hash, salt, password_algorithm, password_parameters)
   VALUES ('ALICE', decode('aa', 'hex'), decode('bb', 'hex'), 'test-only', '{}');
   ```

   Expect a duplicate-key error naming `users_username_unique`. No additional user should be inserted.

6. Create an Alice-only event and a shared event:

   ```sql
   INSERT INTO events(body) VALUES ('{"ciphertext":"alice-only-demo"}')
   RETURNING id AS private_id \gset

   INSERT INTO events(body) VALUES ('{"ciphertext":"shared-demo"}')
   RETURNING id AS shared_id \gset

   INSERT INTO user_events(user_id, event_id)
   VALUES (:alice_id, :private_id), (:alice_id, :shared_id), (:bob_id, :shared_id);

   SELECT e.id, e.body FROM user_events ue JOIN events e ON e.id = ue.event_id
   WHERE ue.user_id = :alice_id ORDER BY e.id;

   SELECT e.id, e.body FROM user_events ue JOIN events e ON e.id = ue.event_id
   WHERE ue.user_id = :bob_id ORDER BY e.id;
   ```

   Alice's query should return both events. Bob's should return only `shared-demo`. The C++ API uses this same user-scoped query; actual HTTP authentication remains future work.

7. Check foreign-key enforcement:

   ```sql
   INSERT INTO user_events(user_id, event_id) VALUES (:alice_id, -1);
   ```

   Expect a foreign-key error, because event `-1` does not exist.

8. Add a dummy session digest and inspect its active state:

   ```sql
   INSERT INTO user_sessions(user_id, token_hash, expires_at)
   VALUES (:alice_id, decode(repeat('ab', 32), 'hex'), now() + interval '1 hour');

   SELECT user_id FROM user_sessions
   WHERE token_hash = decode(repeat('ab', 32), 'hex')
     AND revoked_at IS NULL AND expires_at > now();
   ```

   Expect Alice's ID. This fixed test digest is only for inspection; a real client token must be random and its digest computed by the future authentication layer.

9. Revoke it and repeat the lookup:

   ```sql
   UPDATE user_sessions SET revoked_at = now()
   WHERE token_hash = decode(repeat('ab', 32), 'hex');

   SELECT user_id FROM user_sessions
   WHERE token_hash = decode(repeat('ab', 32), 'hex')
     AND revoked_at IS NULL AND expires_at > now();
   ```

   Expect zero rows. A session whose `expires_at` is in the past is also excluded by this lookup.

10. Exit with `\q`. Restore your normal `HOMESERVER_DB_NAME` in Qt Creator before your next regular run. Repeating the demo inserts in the same database will trigger uniqueness errors; reuse the existing rows or create another disposable demo database.

## Troubleshooting

- **QPSQL driver could not be loaded:** select the Qt 6.11.2 macOS kit and ensure an arm64 Homebrew libpq is installed. Set the library environment variable in the process actually running the program (Terminal or Qt Creator).
- **Connection refused:** verify server host/port and PostgreSQL service status.
- **Password authentication failed:** use the `homeserver` database-role password, not the macOS password or application-user credentials.
- **Permission denied for schema/table:** the database role needs schema creation privileges for tests and ownership/DDL privileges for migrations.
- **Relation already exists during migration:** inspect any manually created conflicting tables. Migration 1 rolls back rather than overwriting unknown tables; do not delete real data to bypass this error.
- **SQL tests pass but `/user/events` returns nothing/404:** no such endpoint is implemented by this change. Test using the SQL above.
