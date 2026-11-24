# Homeserver database

The homeserver stores accounts, login sessions, events, and PQXDH public key bundles in PostgreSQL through Qt SQL's QPSQL driver. `/register`, `/login`, `/user/events`, and `/user/keys` use this database. `/events` and `/events/<id>` store and read events but do not check authentication or ownership, so do not use them as evidence of private event access.

For setup see the README's Homeserver Database Setup. For step-by-step testing see [TESTING.md](../../TESTING.md) and the demonstration guide [TEST.md](TEST.md).

## Schema

- `users`: numeric ID, username, password hash (`BYTEA`), salt (`BYTEA`), creation time. Usernames must be nonempty with no surrounding ASCII spaces, and are unique under PostgreSQL `lower()` using the database's locale. The supplied spelling is preserved. Unicode normalization is not performed.
- `user_sessions`: user ID, unique `token` (`TEXT`, exactly 36 characters: the UUID returned by `/login`), creation/expiration/revocation timestamps. Multiple sessions per user are supported. Expired or revoked sessions never resolve through `sessionUser()`. Revocation is idempotent.
- `events`: ID, JSONB body, creation time. The server treats encrypted event payloads as opaque JSON; database storage does not encrypt or decrypt them.
- `user_events`: unique `(user_id, event_id)` inbox membership. An event can be in several inboxes. Foreign keys reject nonexistent users/events. Deleting a user deletes their sessions and inbox memberships but preserves event bodies. Deleting an event deletes its inbox memberships.
- `user_key_bundles` (v2): one row per user holding their published identity key and signed prekeys as JSONB, replaced on each upload. The bundle must contain a string `identityKey`. Only public keys are stored.
- `user_one_time_prekeys` (v2): one row per unclaimed one-time prekey: user ID, `post_quantum` flag, client-chosen `key_id` (0 to 2^32−1), public key (`BYTEA`), and signature (`BYTEA`, required for post-quantum prekeys, absent for curve prekeys). `(user_id, post_quantum, key_id)` is unique. Deleting a user deletes their bundle and prekeys.
- `homeserver_schema_migrations`: applied migration versions. Version 1 adopts an existing events table and creates the account, session, and inbox tables and indexes. Version 2 adds the two key tables. Each startup applies any missing versions in a single transaction. Startup serializes migrations with a PostgreSQL transaction advisory lock. Schema versions newer than the code supports are rejected.

The database role must own the existing events table and be allowed to create tables/indexes in the active schema. Back up a real database before upgrading it.

### Databases created by older builds

Earlier builds created version 1 with a different layout: `users` also had `password_algorithm` and `password_parameters` columns, and `user_sessions` had a 32-byte `token_hash` instead of `token`. Such a database still reports version 1, so startup changes nothing, but registration then fails with HTTP 500 and sessions cannot be stored.

To check, run `\d user_sessions` in psql. If it shows `token_hash`, the database uses the old layout. For test databases, create a new database. A database with data worth keeping needs a migration that converts the old layout; none exists yet (version 2 only adds key tables and does not repair it). **From now on, applied migrations must never be edited. Schema changes go in a new version.**

## Storage API

`Database` owns a unique Qt SQL connection. Open it once, use it on that same thread, and let its destructor close the connection. Separate instances use separate connections. Connection settings come from `HOMESERVER_DB_HOST`, `HOMESERVER_DB_PORT`, `HOMESERVER_DB_NAME`, `HOMESERVER_DB_USER`, and `HOMESERVER_DB_PASSWORD`.

| Method | Used by | Behavior |
|---|---|---|
| `createUser(username, passwordHash, salt)` | `/register` | Stores precomputed credentials and returns the new ID. The database checks for nonempty values, not cryptographic strength. `/register` hashes passwords with scrypt (`PasswordHasher`) and a random 16-byte salt. |
| `user(username)` | `/register`, `/login` | Case-insensitive lookup returning credentials for internal authentication. Never serialize this structure into an HTTP response. |
| `storeSession(userId, token, expiresAt)` | `/login` | Stores a 36-character token with a future expiry (`/login` uses 24 hours). |
| `sessionUser(token)` | `/user/events`, `/user/keys` | Returns a user ID only for a nonrevoked, unexpired session. |
| `revokeSession(token)` | not yet | Revokes one session; unknown or already-revoked tokens succeed. |
| `assignEvent(userId, eventId)` | not yet | Adds inbox membership; repeating an assignment succeeds without duplication. |
| `addEventForUsers(body, userIds)` | `POST /user/events` | Inserts one event and all memberships atomically. An empty recipient list fails, and any invalid recipient rolls back the whole insert. |
| `eventsForUser(userId, afterId, limit)` | `GET /user/events` | Returns `{id, body}` records in increasing event-ID order. The limit defaults to 100 and must be 1–1000. An empty inbox or a nonexistent positive user ID returns an empty list. `afterId` is for pagination, not a reliable sync cursor; reliable sync needs a separate delivery-sequence design. |
| `addEvent(body)`, `event(id)` | `/events`, `/events/<id>`, `POST /user/events` | Legacy event storage. Neither checks ownership. |
| `storeKeyBundle(userId, bundle, curvePreKeys, pqPreKeys)` | `POST /user/keys` | Atomically replaces the user's bundle and adds one-time prekeys, skipping IDs already in the pool. If `identityKey` changed, first deletes the user's old one-time prekeys (they were signed by the old identity). Returns the remaining pool sizes. |
| `claimKeyBundle(userId)` | `GET /user/keys` | Returns the bundle plus the lowest-ID prekey from each one-time pool, deleting the claimed prekeys in the same transaction. `FOR UPDATE SKIP LOCKED` keeps concurrent fetches from receiving the same prekey. A user with no bundle returns an empty optional with no error. |

All methods clear `lastError()` on entry. Missing lookups return an empty optional with an empty error; SQL and validation failures set the error. SQL errors are for internal logs, not public responses.

### Known gaps

- **Session tokens are stored as plain text.** Anyone who can read `user_sessions` can use a live session. Storing a SHA-256 digest of each token (the original design) would prevent this.
- **The session methods disagree on the token type.** `storeSession()` takes `QString`, while `sessionUser()` and `revokeSession()` take `QByteArray`. Lookups work in testing (callers pass `toUtf8()`), but the types should be made consistent.
- **KDF settings are not stored per user.** The scrypt parameters are fixed in code, so changing them later would invalidate existing password hashes.

## Public key API

Both routes take a JSON body containing the caller's `username` and `authToken` from `/login`, the same as `/user/events`. An invalid token, or a token belonging to a different user, returns **401**. Keys and signatures are standard base64. The server stores and hands out public keys only; it never verifies signatures, since recipients do that when they run PQXDH.

**`POST /user/keys`** publishes the caller's bundle and adds one-time prekeys to their pools:

```json
{
  "username": "bob", "authToken": "<token>",
  "keys": {
    "identityKey": "<X25519 identity public key>",
    "identitySigningKey": "<Ed25519 identity public key, optional>",
    "signedPreKey":   {"id": 10, "publicKey": "...", "signature": "..."},
    "pqSignedPreKey": {"id": 20, "publicKey": "...", "signature": "..."},
    "oneTimePreKeys":   [{"id": 1000, "publicKey": "..."}],
    "pqOneTimePreKeys": [{"id": 2000, "publicKey": "...", "signature": "..."}]
  }
}
```

`identityKey`, `signedPreKey`, and `pqSignedPreKey` are required. The two one-time arrays are optional, hold at most 100 entries each, and are added to the existing pools; an ID already in a pool is skipped. Each key or signature decodes to 1–8192 bytes, and IDs are integers from 0 to 2^32−1. Unrecognized fields are dropped. Uploading a different `identityKey` deletes the caller's old one-time prekeys. Success returns **200** with the remaining pool sizes, which tell the client when to upload more: `{"oneTimePreKeys": 3, "pqOneTimePreKeys": 3}`. A malformed bundle returns **400**.

**`GET /user/keys`** (JSON body: `username`, `authToken`, `recipient`) returns one handshake's worth of the recipient's keys and removes the returned one-time prekeys from the server:

```json
{"keys": {"identityKey": "...", "identitySigningKey": "...",
          "signedPreKey": {...}, "pqSignedPreKey": {...},
          "oneTimePreKey":   {"id": 1000, "publicKey": "..."},
          "pqOneTimePreKey": {"id": 2000, "publicKey": "...", "signature": "..."}}}
```

`oneTimePreKey` or `pqOneTimePreKey` is omitted when that pool is empty; PQXDH then uses only the signed prekeys (`pqSignedPreKey` is the last-resort post-quantum prekey). An unknown recipient, or one who has not uploaded keys, returns **404**.

Limits: any signed-in user can drain another user's one-time prekeys by fetching repeatedly (there is no rate limit), and the owner has no way to read their remaining pool size except by uploading.

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
- **`/user/keys` returns 404 "Recipient Has No Keys":** the recipient has not uploaded a bundle yet.
