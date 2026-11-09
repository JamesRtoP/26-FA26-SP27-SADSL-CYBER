# 26-FA26-SP27-SADSL-CYBER
## Project summary
### One-sentence description of the project
Our project is an end-to-end encrypted chat application utilizing post-quantum encryption standards to remain secure once quantum computers become widespread. Additionally, our application will feature decentralized servers.
### Additional information about the project
The primary objective of our project is to produce an end-to-end encrypted chat application utilizing post-quantum encryption standards, so it remains secure both today and when quantum computing has broken standard encryption schemes. Security and anonymity are paramount to our application, and it must be auditable and anonymously accessible.

The homeserver will handle user registration and initiating connections, then forward messages between users and store message history as a backup for clients changing devices. At no point will the homeserver be able to decrypt stored messages. The client-side application will be responsible for all message decryption, will be the sole storage location for the encryption keys, and will handle device management and device switching.

As of Sprint 2, users can register, log in, start chats, and send and receive messages through a single homeserver. New messages are fetched from the homeserver when a user signs in. Messages are still sent as plain text; encryption, TLS, and federation are planned for upcoming sprints.
## Installation
### Prerequisites
Qt Creator (Qt 6),
OpenSSL 3.5 or newer,
PostgreSQL
### Installation Steps
- Clone the GitHub repository
- Set up the homeserver database (see Homeserver Database Setup below)
- Open CMakeLists.txt from /Code/homeserver in Qt Creator to run the homeserver
- Open CMakeLists.txt from /Code/Q3_Client in Qt Creator to run the client application
- To run two accounts on one computer, set `Q3_CLIENT_DATA_DIR` to a different folder for each client
- See [TESTING.md](TESTING.md) for step-by-step testing instructions
### Homeserver Database Setup
The homeserver stores users, sessions, and events in PostgreSQL using Qt SQL's QPSQL driver.
- Create the database user and database (run as the `postgres` superuser):
  ```sql
  CREATE ROLE homeserver LOGIN PASSWORD 'choose-a-password';
  CREATE DATABASE homeserver OWNER homeserver;
  ```
  The homeserver creates its tables on startup.
- In Qt Creator, open Projects / Project Settings / Project Environment and add:
  - `HOMESERVER_DB_PASSWORD` = the password chosen above
- **macOS only:** Qt's QPSQL driver needs an arm64 `libpq`. The one bundled with the EnterpriseDB PostgreSQL installer is x86_64 only, so install one with `brew install libpq` and also add this to the Run environment:
  - `DYLD_FALLBACK_LIBRARY_PATH` = `/opt/homebrew/opt/libpq/lib`

# Requirements

## Homeserver Requirements

| ID | Requirement | Description | Source | Priority |
|---|---|---|---|---|
| FR-01 | REST API Endpoints | Homeservers shall provide a REST API with POST endpoints for a client's public keys and GET endpoints for retrieving a client's messages. | Team technical decision | Level 0 |
| FR-02 | User Key Retrieval | Homeservers shall retrieve user public keys from other homeservers and provide them to requesting clients for initial key exchange. | Team technical decision | Level 0 |
| FR-03 | New Key Generation | Homeservers shall retrieve new keys from a client application when the majority of that user's public keys have been used. | Team technical decision | Level 1 |

## Client Application Requirements

| ID | Requirement | Description | Source | Priority |
|---|---|---|---|---|
| FR-04 | Homeserver Selection | Client applications shall select their homeserver during user registration. | Client Project Specification | Level 1 |
| FR-05 | Request User Keys | Client applications shall request keys for a specific client from their homeserver before sending them any events. | Team technical decision | Level 1 |
| FR-06 | Event Generation | Client applications will generate events for starting a conversation, sending a message during a conversation, and deleting a message. These events are what will be sent to other clients. | Team technical decision | Level 1 |
| FR-07 | End-to-End Encryption | Client applications shall encrypt all events before sending them to the homeserver and decrypt all their own messages from their homeserver. | Client Project Specification | Level 1 |

## Cryptography Requirements

| ID | Requirement | Description | Source | Priority |
|---|---|---|---|---|
| FR-08 | Initial Key Exchange | Initial key exchange between clients will utilize Post-Quantum Extended Diffie-Hellman (PQXDH). | Team technical decision | Level 0 |
| FR-09 | Message Encryption | Per-message encryption will utilize Sparse Post-Quantum Ratchet (SPQR). | Team technical decision | Level 0 |
## Additional Documentation
- [Class Reports](https://github.com/JamesRtoP/26-FA26-SP27-SADSL-CYBER/tree/main/Reports)
- [Sprints](https://github.com/JamesRtoP/26-FA26-SP27-SADSL-CYBER/tree/main/Sprints)
- [Testing Guide](TESTING.md)
- [Homeserver Database](Code/homeserver/DATABASE.md)
- [Client Database](Code/Q3_Client/DATABASE.md)
## License
- [GNU General Public License](https://github.com/JamesRtoP/26-FA26-SP27-SADSL-CYBER/blob/main/LICENSE)
