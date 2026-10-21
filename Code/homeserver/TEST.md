# PostgreSQL Database Demo — Tester Guide

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

**Expected:** `CREATE DATABASE` succeeds. This assumes the `homeserver` role already exists. If the demo database already exists, inspect and reuse it if appropriate; do not delete existing data just to repeat the demonstration.

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

**Expected:** startup succeeds and logs that the server is listening on port `8080`. Startup applies the database migration. A listening log alone is not a passing test; complete the functional requests below.

**Presentation:** “The homeserver connects to PostgreSQL and initializes its schema automatically.”

## 3. Verify the HTTP service

In a second Terminal window:

```bash
curl -i --max-time 10 http://127.0.0.1:8080/health
```

**Expected:** HTTP `200` with body `OK`.

## 4. Prove that an event survives a restart

Create an identifiable event:

```bash
curl -i --max-time 10 \
  -H 'Content-Type: application/json' \
  -d '{"demo":"client-persistence","payload":"sample event"}' \
  http://127.0.0.1:8080/events
  ```
  
Run:

```sql
\pset pager off
SELECT current_database();
\dt
SELECT version, applied_at FROM homeserver_schema_migrations ORDER BY version;
SELECT id, body, created_at FROM events ORDER BY id DESC LIMIT 5;
```

**Expected:**

- The database is `homeserver_demo_test`.
- Tables include `users`, `user_sessions`, `events`, `user_events`, and `homeserver_schema_migrations`.
- Migration version `1` is present.
- The event created through HTTP is visible with its ID, JSON body, and timestamp.

## 7. Run the account, inbox, and session demonstration

Keep the same `psql` session. Run the companion script using its absolute local path:

```sql
\i '/absolute/path/to/database-client-demo.sql'
```

For a paced demonstration, open the SQL file and run its numbered sections separately in the same session. Keep the opening `BEGIN` and final `ROLLBACK`; later statements use IDs captured by earlier statements.

The script refuses to proceed against a database whose name differs from `homeserver_demo_test`. Its credential values and token digests are deliberately fake fixtures, not real password hashes or usable authentication tokens.

| Check | Expected result | What to explain |
|---|---|---|
| Schema version | `1` | Startup initialized the schema. |
| Create Alice and Bob | Two account records with credential byte lengths | Account credentials have dedicated storage fields. |
| Case-only duplicate username | `PASS: duplicate username rejected` notice | Uniqueness is enforced by the database. |
| Private and shared inbox assignments | Alice has two events; Bob has only the shared event | Inbox relationships support separate recipients and shared events. |
| Repeat Bob's shared-event assignment | Membership count `1` | Duplicate assignments do not duplicate inbox membership. |
| Insert event with nonexistent recipient | `PASS: invalid recipient rejected and new event rolled back` notice | The transaction leaves no partially inserted event. |
| Active session lookup | `active_before = 1` | An active session record matches the eligibility query. |
| Revoke session and repeat lookup | `active_after = 0` | Revocation excludes the session. |
| Expired session lookup | `active_bob_sessions = 0` | Expired sessions are excluded. |

The expected constraint violations are caught inside the script and reported as `PASS` notices. An uncaught `ERROR`, a `FAIL` exception, or a result that differs from the table above is a failed or incomplete check; do not describe it as passing.

The script finishes with `ROLLBACK`, removing its sample rows. PostgreSQL-generated ID sequences can advance even after rollback, so gaps in event IDs are normal. The earlier HTTP event was committed separately and remains stored.

**Presentation:** “The schema enforces unique accounts and valid relationships. Inbox queries distinguish recipients, and session queries exclude revoked or expired sessions. These are database-level checks; authenticated endpoints will need to call the corresponding C++ methods.”

## 8. Finish and restore settings

```sql
\q
```

- Stop the homeserver process you started.
- Restore Qt Creator's previous `HOMESERVER_DB_NAME` and other changed Run Environment settings.
- Keep the demo database for inspection. Do not drop a database without confirming it is disposable.
- If the SQL script stopped early while running interactively, run `ROLLBACK;` before retrying. If the connection is closed, its uncommitted transaction is rolled back automatically.
- To repeat the SQL demo, reconnect to the demo database and rerun the script. Repeating the HTTP POST creates another committed event, which is expected.

## Troubleshooting

| Symptom | Action |
|---|---|
| QPSQL driver cannot load | Verify the selected Qt kit includes QPSQL and its libpq architecture matches. Check the Run Environment library path. |
| PostgreSQL connection refused | Check PostgreSQL is running and verify host/port. |
| Password authentication failed | Use the PostgreSQL `homeserver` role password, not a macOS or chat-account password. |
| Schema permission error | Confirm the role owns the demo database and can create tables in the active schema. |
| HTTP connection refused | Inspect Qt Creator startup output and confirm the process is running on port `8080`. |
| Event missing after restart | Check the recorded ID, branch, database name, host, and port are unchanged. |
| Demo usernames already exist | A prior run may have committed fixture data. Inspect it or use another deliberately prepared demo environment; do not delete unknown records blindly. |
| `/register`, `/login`, or `/user/events` returns 404 | Those endpoints are not implemented in this branch. They are not part of this demo. |
