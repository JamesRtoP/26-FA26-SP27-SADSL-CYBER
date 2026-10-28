# PostgreSQL Database Demo — Tester Guide

This guide demonstrates the homeserver's PostgreSQL storage: schema setup, persistence across restarts, accounts, inboxes, and sessions. For testing the whole application (client app and client database included), see [TESTING.md](../../TESTING.md).

## 1. Create a dedicated demo database

Connect as the PostgreSQL administrator:

```bash
/Library/PostgreSQL/18/bin/psql -h localhost -U postgres -d postgres -W
```

Run:

```sql
CREATE DATABASE homeserver_demo_test OWNER homeserver;
\q
```

**Expected:** `CREATE DATABASE` succeeds. This assumes the `homeserver` role already exists (see the README's Homeserver Database Setup).

**If `homeserver_demo_test` already exists**, check whether an older build created it:

```sql
\c homeserver_demo_test
\d user_sessions
```

If the table has a `token_hash` column instead of `token`, the database uses an outdated layout, and registration and this demo will fail. Because it is the disposable demo database, recreate it:

```sql
\c postgres
DROP DATABASE homeserver_demo_test;
CREATE DATABASE homeserver_demo_test OWNER homeserver;
```

Never drop any other database to repeat this demonstration.

## 2. Configure and start the homeserver

1. Open `Code/homeserver/CMakeLists.txt` in Qt Creator.
2. Select the working Qt kit and use a build directory outside the source directory.
3. In the project's **Run Environment**, set:

   | Variable | Value |
   |---|---|
   | `HOMESERVER_DB_HOST` | `localhost` |
   | `HOMESERVER_DB_PORT` | `5432` |
   | `HOMESERVER_DB_NAME` | `homeserver_demo_test` |
   | `HOMESERVER_DB_USER` | `homeserver` |
   | `HOMESERVER_DB_PASSWORD` | The existing PostgreSQL role password |
   | `DYLD_FALLBACK_LIBRARY_PATH` | `/opt/homebrew/opt/libpq/lib` if needed on Apple Silicon |

4. Build and run the homeserver.

**Expected:** startup succeeds and logs `Listening on port 8080`. Startup applies the database migration. A listening log alone is not a passing test; complete the functional requests below.

**Presentation:** “The homeserver connects to PostgreSQL and initializes its schema automatically.”

## 3. Verify the HTTP service

In a second Terminal window:

```bash
curl -i --max-time 10 http://127.0.0.1:8080/health
```

**Expected:** HTTP `200` with body `OK`.

## 4. Prove that data survives a restart

Create an identifiable event:

```bash
curl -i --max-time 10 \
  -H 'Content-Type: application/json' \
  -d '{"demo":"client-persistence","payload":"sample event"}' \
  http://127.0.0.1:8080/events
```

**Expected:** HTTP `201` with a JSON response containing the generated event ID, for example `{"id":42}`. Record the actual ID; do not assume it is `42` or starts at `1`.

Retrieve it, replacing `42` with that ID:

```bash
curl -i --max-time 10 http://127.0.0.1:8080/events/42
```

**Expected:** HTTP `200` containing the submitted JSON fields. JSON field order and whitespace may differ.

Register an account and log in:

```bash
curl -i --max-time 10 -H 'Content-Type: application/json' \
  -d '{"username":"demo_user","password":"demo-password-1"}' http://127.0.0.1:8080/register
curl -i --max-time 10 -H 'Content-Type: application/json' \
  -d '{"username":"demo_user","password":"demo-password-1"}' http://127.0.0.1:8080/login
```

**Expected:** registration returns `201` `User Created`; login returns `200` with `{"authToken":"…"}`. If `demo_user` already exists from a previous run, registration returns `400` `Username Taken`, which is fine; continue with the login.

1. Stop the homeserver in Qt Creator.
2. Run the health request again. It should fail to connect once the process has stopped. If it still responds, identify the other process before continuing.
3. Restart the homeserver with the same database configuration.
4. Repeat the health request, the GET request for the recorded event ID, and the login request.

**Expected:** health returns `200 / OK`, the event still returns `200` with the same submitted fields, and login still returns `200` with a new token.

**Pass criterion:** an event and an account created through the application remain usable after the application process restarts.

**Presentation:** “The event and the account survive a server restart because they are stored in PostgreSQL rather than temporary in-memory lists.”

The sample event is plaintext test data. This does not demonstrate encryption or private event access.

## 5. Inspect the schema and persisted data

Connect as the application's database role. Start psql from the repository's root folder so step 6 can find the demo script:

```bash
/Library/PostgreSQL/18/bin/psql -h localhost -U homeserver -d homeserver_demo_test -W
```

Run:

```sql
\pset pager off
SELECT current_database();
\dt
SELECT version, applied_at FROM homeserver_schema_migrations ORDER BY version;
SELECT id, body, created_at FROM events ORDER BY id DESC LIMIT 5;
SELECT id, username, octet_length(password_hash) AS hash_bytes, octet_length(salt) AS salt_bytes FROM users;
SELECT user_id, created_at, expires_at, revoked_at FROM user_sessions ORDER BY id DESC LIMIT 5;
```

**Expected:**

- The database is `homeserver_demo_test`.
- Tables include `users`, `user_sessions`, `events`, `user_events`, and `homeserver_schema_migrations`.
- Migration version `1` is present.
- The event created through HTTP is visible with its ID, JSON body, and timestamp.
- `demo_user` has a 32-byte password hash and a 16-byte salt; the password itself is never stored.
- Each login created a session that expires about 24 hours after it was created.

## 6. Run the account, inbox, and session demonstration

Keep the same `psql` session and run the companion script:

```sql
\i Code/homeserver/database-demo.sql
```

If psql was started from another folder, use the script's absolute path instead. For a paced demonstration, open the SQL file and run its numbered sections separately in the same session. Keep the opening `BEGIN` and final `ROLLBACK`; later statements use IDs captured by earlier statements.

The script refuses to proceed against a database whose name differs from `homeserver_demo_test`. Its credential values and session tokens are deliberately fake fixtures, not real password hashes or usable tokens.

| Check | Expected result | What to explain |
|---|---|---|
| 1. Schema version | `schema_version = 1` | Startup initialized the schema. |
| 2. Create Alice and Bob | `demo_alice` and `demo_bob`, each with `hash_bytes = 32` and `salt_bytes = 16` | Account credentials have dedicated storage fields. |
| 3. Case-only duplicate username | `PASS: duplicate username rejected` notice | Uniqueness is enforced by the database. |
| 4. Private and shared inbox assignments | Alice's inbox has two events; Bob's has only the shared event | Inbox relationships support separate recipients and shared events. |
| 5. Repeat Bob's shared-event assignment | `bob_shared_memberships = 1` | Duplicate assignments do not duplicate inbox membership. |
| 6. Insert event with nonexistent recipient | `PASS: invalid recipient rejected and new event rolled back` notice | The transaction leaves no partially inserted event. |
| 7. Active session lookup | `active_before = 1` | An active session record matches the eligibility query. |
| 8. Revoke session and repeat lookup | `active_after = 0` | Revocation excludes the session. |
| 9. Expired session lookup | `active_bob_sessions = 0` | Expired sessions are excluded. |

The script ends by printing `Demo finished; all sample rows were rolled back.` An `ERROR`, a `FAIL` exception, or a result that differs from the table above is a failed or incomplete check; do not describe it as passing. Because the script stops at its first error, run `ROLLBACK;` before retrying.

The script finishes with `ROLLBACK`, removing its sample rows. PostgreSQL-generated ID sequences can advance even after rollback, so gaps in event IDs are normal. The earlier HTTP event and `demo_user` were committed separately and remain stored.

**Presentation:** “The schema enforces unique accounts and valid relationships. Inbox queries distinguish recipients, and session queries exclude revoked or expired sessions. Registration and login already use these tables; authenticated message endpoints will use the inbox and session methods next.”

## 7. Finish and restore settings

```sql
\q
```

- Stop the homeserver process you started.
- Restore Qt Creator's previous `HOMESERVER_DB_NAME` and other changed Run Environment settings.
- Keep the demo database for inspection. Do not drop a database without confirming it is disposable.
- If the connection is closed during the script, its uncommitted transaction is rolled back automatically.
- To repeat the SQL demo, reconnect to the demo database and rerun the script. Repeating the HTTP POST creates another committed event, which is expected.

## Troubleshooting

| Symptom | Action |
|---|---|
| QPSQL driver cannot load | Verify the selected Qt kit includes QPSQL and its libpq architecture matches. Check the Run Environment library path. |
| PostgreSQL connection refused | Check PostgreSQL is running and verify host/port. |
| Password authentication failed | Use the PostgreSQL `homeserver` role password, not a macOS or chat-account password. |
| Schema permission error | Confirm the role owns the demo database and can create tables in the active schema. |
| HTTP connection refused | Inspect Qt Creator startup output and confirm the process is running on port `8080`. |
| `/register` or `/login` returns `500`, or the script reports `column "token" … does not exist` | The demo database was created by an older build. Recreate it (section 1). |
| `\i` reports `No such file or directory` | Start psql from the repository root, or give the script's absolute path. |
| Event missing after restart | Check the recorded ID, branch, database name, host, and port are unchanged. |
| Script stops with `Refusing to run against …` | Connect to `homeserver_demo_test`, not your main database. |
| `/user/events` returns 404 | Chat and message endpoints are not implemented yet. They are not part of this demo. |
