# Testing Q3-Chat

These steps let you test the homeserver, the client app, and both databases on your own computer. Each step lists what you should see. A result that differs from **Expected** is a failed check.

## What works today

| Feature | Can be tested? |
|---|---|
| Homeserver health check, event storage, restart persistence | Yes (sections 3–4) |
| Registration and login, stored in PostgreSQL | Yes, with curl or the client app (sections 4–5) |
| Client local database (created at login) | Yes (sections 6–7) |
| Starting a chat and sending messages | **Not yet.** The homeserver does not implement `/user/events`, so the client shows an error |
| Encryption | **Not yet.** Nothing is encrypted at this stage, so don't use real passwords or messages |

## 1. Prerequisites

- **Qt 6.10 or newer** with Qt Creator, including the Qt HTTP Server and Qt SQL modules (Qt 6.11.2 is tested)
- **PostgreSQL** (version 18 is tested), running locally
- **OpenSSL 3**
- **curl** (included with macOS, Linux, and Windows 10+)
- Optional: **sqlite3** for inspecting the client database (included with macOS)

**macOS (Apple Silicon):** Qt's PostgreSQL driver needs an arm64 `libpq`. The one from the PostgreSQL installer is x86_64 only, so run `brew install libpq`.
**Windows:** the PostgreSQL `bin` folder (for example `C:\Program Files\PostgreSQL\18\bin`) must be on the `PATH` of the running homeserver so Qt can find `libpq.dll`.

## 2. Create a test database

Use a **new, empty** database for testing. Databases created by older versions of this branch have an outdated table layout, and registration fails on them.

Connect as the PostgreSQL administrator:

```bash
psql -h localhost -U postgres -d postgres -W
```

```sql
-- Skip CREATE ROLE if the homeserver role already exists.
CREATE ROLE homeserver LOGIN PASSWORD 'choose-a-password';
CREATE DATABASE homeserver_test OWNER homeserver;
\q
```

**Expected:** `CREATE ROLE` and `CREATE DATABASE` both succeed. The homeserver creates its tables the first time it starts.

## 3. Start the homeserver

1. In Qt Creator, open `Code/homeserver/CMakeLists.txt` and select your Qt 6 kit.
2. Under **Projects → Run → Environment**, add:

   | Variable | Value |
   |---|---|
   | `HOMESERVER_DB_NAME` | `homeserver_test` |
   | `HOMESERVER_DB_PASSWORD` | the password from step 2 |
   | `DYLD_FALLBACK_LIBRARY_PATH` | `/opt/homebrew/opt/libpq/lib` (**macOS only**) |

   Optional, if your database isn't on the defaults: `HOMESERVER_DB_HOST` (default `localhost`), `HOMESERVER_DB_PORT` (`5432`), `HOMESERVER_DB_USER` (`homeserver`).
3. Click **Run**.

**Expected:** a "Simple Homeserver" window opens, and the Application Output shows `Listening on port 8080`. If it shows `Failed to connect to the database`, see Troubleshooting.

## 4. Test the homeserver with curl

Run these in a terminal while the homeserver is running. On Windows, use Git Bash, or type `curl.exe` in PowerShell and swap the quoting.

| # | Command | Expected |
|---|---|---|
| 4.1 | `curl -i http://127.0.0.1:8080/health` | `200`, body `OK` |
| 4.2 | `curl -i -H 'Content-Type: application/json' -d '{"username":"alice","password":"test-password-1"}' http://127.0.0.1:8080/register` | `201`, `User Created` |
| 4.3 | Repeat 4.2 with `"username":"ALICE"` | `400`, `Username Taken` (usernames are case-insensitive) |
| 4.4 | `curl -i -H 'Content-Type: application/json' -d '{"username":"alice","password":"test-password-1"}' http://127.0.0.1:8080/login` | `200`, body `{"authToken":"…"}` with a 36-character token |
| 4.5 | Repeat 4.4 with `"password":"wrong-password"` | `400`, `Incorrect Username or Password` |
| 4.6 | Repeat 4.4 with `"username":"nobody"` | `400`, the same message as 4.5 (it does not reveal which part was wrong) |
| 4.7 | `curl -i -H 'Content-Type: application/json' -d '{"demo":"persistence"}' http://127.0.0.1:8080/events` | `201`, body like `{"id":1}`. Note the ID |
| 4.8 | `curl -i http://127.0.0.1:8080/events/<id from 4.7>` | `200`, body contains `"demo":"persistence"` |
| 4.9 | `curl -i http://127.0.0.1:8080/events/999999` | `404`, `Event not found` |

**Restart check:** stop the homeserver in Qt Creator, run it again, then repeat 4.4 and 4.8.
**Expected:** login still works and the event is still returned, because both are stored in PostgreSQL.

## 5. Test the client app

Keep the homeserver running.

1. In Qt Creator, open `Code/Q3_Client/CMakeLists.txt`, select the same kit, and click **Run**. A "Q3-Chat" window opens on the **SIGN IN** screen.
2. Enter a new username (for example `bob`) and a password, then click **Create account**.
   **Expected:** the message `Account created. You can sign in now.`
3. Click **Create account** again with the same username.
   **Expected:** the error `Username Taken`.
4. Click **Sign in** with a wrong password.
   **Expected:** the error `Incorrect Username or Password`.
5. Click **Sign in** with the correct password.
   **Expected:** the window switches to the chat screen, with a **Messages** sidebar on the left.
6. Click **+** (New conversation) in the sidebar, enter `alice`, and click **Start chat**.
   **Expected (known limitation):** the dialog stays open and shows an error such as `Failed to start chat`, and no conversation is added. The server route for chats is not implemented yet.

## 6. Check the client's local database

Logging in (step 5.5) creates a database file for that account:

| Platform | File |
|---|---|
| macOS | `~/Library/Application Support/appQ3_Client/bob.sqlite` |
| Windows | `%APPDATA%\appQ3_Client\bob.sqlite` |
| Linux | `~/.local/share/appQ3_Client/bob.sqlite` |

The file name is the lowercase username. To keep test files somewhere else, set `Q3_CLIENT_DATA_DIR` in the client's Run Environment before logging in.

```bash
ls -l ~/Library/Application\ Support/appQ3_Client/      # macOS/Linux: check permissions
sqlite3 ~/Library/Application\ Support/appQ3_Client/bob.sqlite
```
```sql
.tables
SELECT version FROM schema_migrations;
SELECT * FROM contacts;
SELECT * FROM messages;
.quit
```

**Expected:**
- Permissions are `-rw-------`, meaning only your user account can read the file.
- `.tables` lists `contacts messages schema_migrations settings`, and the version is `1`.
- `contacts` and `messages` are empty, because chats cannot start yet (step 5.6).
- Logging in as a second user creates a second, separate file.

## 7. Run the client database unit tests

These tests need no server or PostgreSQL, because each one uses a temporary file.

**Qt Creator:** open `Code/Q3_Client/CMakeLists.txt`. Under **Projects → Build → CMake**, set `Q3_CLIENT_BUILD_TESTS` to `ON` and click **Run CMake**. Then select the `tst_clientdatabase` run target and click **Run**.

**Command line:**
```bash
cmake -S Code/Q3_Client -B build-client-tests -DQ3_CLIENT_BUILD_TESTS=ON -DCMAKE_PREFIX_PATH=<path to your Qt kit, e.g. ~/Qt/6.11.2/macos>
cmake --build build-client-tests
ctest --test-dir build-client-tests --output-on-failure
```
Keep build folders out of the repository, or delete them afterwards.

**Expected:** `100% tests passed`. Running `tst_clientdatabase` directly prints one `PASS` line per test and ends with `Totals: 17 passed, 0 failed` (15 tests plus Qt's two setup and cleanup entries).

## 8. Inspect the homeserver database

```bash
psql -h localhost -U homeserver -d homeserver_test -W
```
```sql
\dt
SELECT id, username, octet_length(password_hash) AS hash_bytes, octet_length(salt) AS salt_bytes FROM users;
SELECT user_id, created_at, expires_at, revoked_at FROM user_sessions;
SELECT id, body FROM events ORDER BY id;
\q
```

**Expected:**
- Tables: `events`, `homeserver_schema_migrations`, `user_events`, `user_sessions`, `users`.
- Each user has a 32-byte password hash and a 16-byte salt. Passwords are never stored.
- Each successful login added a session that expires about 24 hours after it was created.
- The event from step 4.7 is listed.

## Troubleshooting

| Symptom | Fix |
|---|---|
| `QPSQL driver could not be loaded` | macOS: install `libpq` with Homebrew and set `DYLD_FALLBACK_LIBRARY_PATH` (step 3). Windows: add the PostgreSQL `bin` folder to `PATH` |
| `password authentication failed` | Use the `homeserver` database role's password, not your computer or chat password |
| `Connection refused` from the homeserver | Start PostgreSQL and check the host and port |
| `curl: (7) Failed to connect` | The homeserver isn't running, or port 8080 is in use by another program |
| Register or login returns `500` | You're likely using a database created by an older build. Create a new database (step 2) and point `HOMESERVER_DB_NAME` at it |
| Client login shows `Could not open local message storage` | The client can't write its data folder. Check permissions, or set `Q3_CLIENT_DATA_DIR` to a writable folder |
| Start chat shows an error | Expected for now (see "What works today") |

## Clean up

- Stop both apps.
- Remove the test database when you're done (`DROP DATABASE homeserver_test;` as `postgres`). Only drop databases you created for testing.
- Delete the client test files (for example `bob.sqlite`) from the folder in section 6.
- Reset any Run Environment values you changed in Qt Creator.
