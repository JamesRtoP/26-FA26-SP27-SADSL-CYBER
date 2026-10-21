# PostgreSQL Database Demo — Tester Guide

**Branch:** `feat/qt-sql-server-connection`  
**Duration:** approximately 7–10 minutes  
**Companion file:** [database-client-demo.sql](database-client-demo.sql)

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