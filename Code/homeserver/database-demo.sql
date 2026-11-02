-- Homeserver PostgreSQL demonstration: accounts, inboxes, and sessions.
-- Run from psql connected to homeserver_demo_test: \i Code/homeserver/database-demo.sql
-- Every change runs in one transaction and is rolled back at the end. Credential bytes
-- and session tokens below are fake fixtures, not real password hashes or usable tokens.
-- Expected constraint violations are caught and reported as PASS notices.

\set ON_ERROR_STOP on
\pset pager off

-- Refuse to touch any database except the disposable demo database.
DO $$
BEGIN
    IF current_database() <> 'homeserver_demo_test' THEN
        RAISE EXCEPTION 'Refusing to run against %; connect to homeserver_demo_test', current_database();
    END IF;
END $$;

BEGIN;

-- 1. Schema version: startup applied migration 1.
SELECT MAX(version) AS schema_version FROM homeserver_schema_migrations;

-- 2. Create two accounts with dedicated credential fields.
INSERT INTO users(username, password_hash, salt)
VALUES ('demo_alice', decode(repeat('aa', 32), 'hex'), decode(repeat('11', 16), 'hex'))
RETURNING id AS alice_id \gset
INSERT INTO users(username, password_hash, salt)
VALUES ('demo_bob', decode(repeat('bb', 32), 'hex'), decode(repeat('22', 16), 'hex'))
RETURNING id AS bob_id \gset
SELECT username, octet_length(password_hash) AS hash_bytes, octet_length(salt) AS salt_bytes
FROM users WHERE id IN (:alice_id, :bob_id) ORDER BY id;

-- 3. A username differing only in case is rejected by the unique index.
DO $$
BEGIN
    INSERT INTO users(username, password_hash, salt)
    VALUES ('DEMO_ALICE', decode('aa', 'hex'), decode('bb', 'hex'));
    RAISE EXCEPTION 'FAIL: duplicate username was accepted';
EXCEPTION WHEN unique_violation THEN
    RAISE NOTICE 'PASS: duplicate username rejected';
END $$;

-- 4. A private event for Alice and a shared event for both users.
INSERT INTO events(body) VALUES ('{"ciphertext":"alice-only-demo"}') RETURNING id AS private_id \gset
INSERT INTO events(body) VALUES ('{"ciphertext":"shared-demo"}') RETURNING id AS shared_id \gset
INSERT INTO user_events(user_id, event_id)
VALUES (:alice_id, :private_id), (:alice_id, :shared_id), (:bob_id, :shared_id);
SELECT 'alice' AS inbox, e.id, e.body FROM user_events ue JOIN events e ON e.id = ue.event_id
WHERE ue.user_id = :alice_id
UNION ALL
SELECT 'bob', e.id, e.body FROM user_events ue JOIN events e ON e.id = ue.event_id
WHERE ue.user_id = :bob_id
ORDER BY inbox, id;

-- 5. Repeating an assignment does not duplicate inbox membership.
INSERT INTO user_events(user_id, event_id) VALUES (:bob_id, :shared_id) ON CONFLICT DO NOTHING;
SELECT COUNT(*) AS bob_shared_memberships FROM user_events
WHERE user_id = :bob_id AND event_id = :shared_id;

-- 6. An event with a nonexistent recipient is rolled back as a whole, like addEventForUsers().
DO $$
BEGIN
    BEGIN
        WITH new_event AS (
            INSERT INTO events(body) VALUES ('{"ciphertext":"bad-recipient-demo"}') RETURNING id
        )
        INSERT INTO user_events(user_id, event_id) SELECT -1, id FROM new_event;
        RAISE EXCEPTION 'FAIL: invalid recipient was accepted';
    EXCEPTION WHEN foreign_key_violation THEN
        NULL;
    END;
    IF EXISTS (SELECT 1 FROM events WHERE body = '{"ciphertext":"bad-recipient-demo"}') THEN
        RAISE EXCEPTION 'FAIL: event remained after its recipient was rejected';
    END IF;
    RAISE NOTICE 'PASS: invalid recipient rejected and new event rolled back';
END $$;

-- 7. Session lookup uses the same filter as Database::sessionUser().
INSERT INTO user_sessions(user_id, token, expires_at)
VALUES (:alice_id, '00000000-0000-4000-8000-00000000a11c', now() + interval '1 hour');
SELECT COUNT(*) AS active_before FROM user_sessions
WHERE token = '00000000-0000-4000-8000-00000000a11c' AND revoked_at IS NULL AND expires_at > now();

-- 8. Revocation excludes the session.
UPDATE user_sessions SET revoked_at = COALESCE(revoked_at, now())
WHERE token = '00000000-0000-4000-8000-00000000a11c';
SELECT COUNT(*) AS active_after FROM user_sessions
WHERE token = '00000000-0000-4000-8000-00000000a11c' AND revoked_at IS NULL AND expires_at > now();

-- 9. An expired session is excluded.
INSERT INTO user_sessions(user_id, token, created_at, expires_at)
VALUES (:bob_id, '00000000-0000-4000-8000-000000000b0b', now() - interval '2 hours', now() - interval '1 hour');
SELECT COUNT(*) AS active_bob_sessions FROM user_sessions
WHERE user_id = :bob_id AND revoked_at IS NULL AND expires_at > now();

ROLLBACK;
\echo 'Demo finished; all sample rows were rolled back.'
