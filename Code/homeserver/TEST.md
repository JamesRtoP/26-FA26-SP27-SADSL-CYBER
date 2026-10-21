# PostgreSQL Database Demo — Tester Guide

**Branch:** `feat/qt-sql-server-connection`  
**Duration:** approximately 7–10 minutes  
**Companion file:** [database-client-demo.sql](database-client-demo.sql)

## Purpose

Demonstrate that the homeserver persists events in PostgreSQL and that the database schema supports account records, user inboxes, session expiry/revocation, uniqueness constraints, and transactional rollback.

The HTTP portion exercises the application's event-storage path. The SQL portion exercises PostgreSQL directly; it does not test the C++ account/session/inbox methods or authenticated HTTP access. Message encryption, registration/login endpoints, and `/user/events` are outside this branch's demo.

## Prerequisites

- A local checkout with the database branch available.
- Qt Creator and a compatible Qt kit (CMake requires Qt 6.10 or later), including Quick, HttpServer, Sql, and the QPSQL driver.
- PostgreSQL running locally; an existing `homeserver` login role and its password.
- A working PostgreSQL client (`psql`) and `curl`.
- On Apple Silicon, a compatible arm64 `libpq` accessible to the QPSQL driver. The branch documentation uses Homebrew's `/opt/homebrew/opt/libpq/lib`.
- Save this guide and `database-client-demo.sql` in the same local folder.

Commands below use the PostgreSQL 18 macOS installation path. Substitute your installed `psql` path if different. Enter passwords at prompts or securely in Qt Creator; do not put them in recorded commands.

## 1. Select the branch

From Terminal:

```bash
cd /Users/aidenvezzetti/School/Fall2026/CS421/26-FA26-SP27-SADSL-CYBER
git status --short
git switch feat/qt-sql-server-connection
git branch --show-current
git rev-parse HEAD
```

**Expected:** the selected branch is `feat/qt-sql-server-connection`. Record the commit SHA with the test results. Preserve any existing local changes; do not force checkout or reset the repository.

The repository directory is `Code/homeserver` with an uppercase `C`.

## 2. Create a dedicated demo database

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

## 3. Configure and start the homeserver

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

## 4. Verify the HTTP service

In a second Terminal window:

```bash
curl -i --max-time 10 http://127.0.0.1:8080/health
```

**Expected:** HTTP `200` with body `OK`.

## 5. Prove that an event survives a restart

Create an identifiable event:

```bash
curl -i --max-time 10 \
  -H 'Content-Type: application/json' \
  -d '{"demo":"client-persistence","payload":"sample event"}' \
  http://127.0.0.1:8080/events