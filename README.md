# 26-FA26-SP27-SADSL-CYBER
## Project summary
### One-sentence description of the project
Our project is an end-to-end encrypted chat application utilizing post-quantum encryption standards to remain secure once quantum computers become widespread. Additionally, our application will feature decentralized servers.
### Additional information about the project
The primary objective of our project is to produce an end-to-end encrypted chat application utilizing post-quantum encryption standards, so it remains secure both today and when quantum computing has broken standard encryption schemes. Security and anonymity are paramount to our application, and it must be auditable and anonymously accessible.

The homeserver will handle user registration and initiating connections, then forward messages between users and store message history as a backup for clients changing devices. At no point will the homeserver be able to decrypt stored messages. The client-side application will be responsible for all message decryption, will be the sole storage location for the encryption keys, and will handle device management and device switching.

As of Sprint 2, users can register, log in, start chats, and send/receive messages through a single homeserver. New messages are fetched from the homeserver when a user signs in. Messages are still sent as plain text; encryption, TLS, and federation are planned for upcoming sprints.
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
- Follow [TESTING.md](TESTING.md) to confirm the homeserver and client are working
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
| FR-01 | REST API Endpoints | Homeservers shall provide a REST API with POST endpoints for uploading a client's public keys and events. GET endpoints must also be provided for retrieving public keys and a client's queued events. | Team technical decision | Level 0 |
| FR-02 | User Key Retrieval | Homeservers shall retrieve user public keys from other homeservers and provide them to requesting clients for the initial key exchange. | Team technical decision | Level 0 |
| FR-03 | New Key Generation | Homeservers shall request new one-time prekeys from a client application when most of that user's uploaded prekeys have been used. | Team technical decision | Level 1 |
| FR-10 | Account Registration and Authentication | Homeservers shall register accounts using only a username and password, store each password as a salted, memory-hard hash, and issue expiring, revocable session tokens at login. | Client Project Specification (anonymity) | Level 0 |
| FR-11 | Offline Event Queueing | Homeservers shall keep encrypted events in each recipient's inbox until the recipient's client retrieves them. A recipient being offline shall not be treated as a delivery failure. | Team technical decision | Level 0 |

## Client Application Requirements

| ID | Requirement | Description | Source | Priority |
|---|---|---|---|---|
| FR-04 | Homeserver Selection | Client applications shall let the user select their homeserver during registration. | Client Project Specification | Level 1 |
| FR-05 | Request User Keys | Client applications shall request a user's public keys from their homeserver before sending that user any events. | Team technical decision | Level 0 |
| FR-06 | Event Generation | Client applications shall generate events for starting a conversation, sending a message, and deleting a message. These events are what is sent to other clients. | Team technical decision | Level 0 |
| FR-07 | End-to-End Encryption | Client applications shall encrypt all events before sending them to the homeserver and shall decrypt events retrieved from the homeserver only on the device. | Client Project Specification | Level 0 |
| FR-14 | Local Message History | Client applications shall store message history and private keys only on the device and shall let users delete a message for themselves or for both participants. | Team technical decision | Level 1 |

## Cryptography Requirements

| ID | Requirement | Description | Source | Priority |
|---|---|---|---|---|
| FR-08 | Initial Key Exchange | The initial key exchange between clients shall use Post-Quantum Extended Diffie-Hellman (PQXDH) as specified by Signal. | Team technical decision | Level 0 |
| FR-09 | Message Encryption | Per-message keys shall be derived with the Sparse Post-Quantum Ratchet (SPQR) together with the Double Ratchet. | Team technical decision | Level 0 |

## Federation and Device Management Requirements

| ID | Requirement | Description | Source | Priority |
|---|---|---|---|---|
| FR-12 | Server-to-Server Routing | Homeservers shall locate the homeserver responsible for a domain-qualified username, authenticate to it, and forward key requests and encrypted events to it, retrying while the remote homeserver is unreachable. | Client Project Specification (decentralization) | Level 1 |
| FR-13 | New Device Linking | Client applications shall link a new device only after the user approves it on an existing device, and shall transfer contacts and keys between the devices over an end-to-end encrypted channel. Private keys shall never be uploaded to a homeserver in readable form. | Team technical decision | Level 1 |

## Non-Functional Requirements

| ID | Requirement | Description | Verification |
|---|---|---|---|
| NFR-1 | Privacy | The system shall keep all messages encrypted while they are on a homeserver; they shall only be decrypted on users' devices. | Inspect stored events and network traffic to confirm only ciphertext reaches the server |
| NFR-2 | Containerization | The homeserver shall be containerized (e.g., Docker) so that deploying it is simple. | Start a homeserver and its database from the container image on a clean machine |
| NFR-3 | Scalability | The system shall be deployable as decentralized servers, so that a new homeserver can be added and exchange messages with existing homeservers without any change to their configuration. | Add a homeserver during testing and send messages to users on the existing homeservers without editing their configuration |
| NFR-4 | Reliability | When one homeserver is taken offline, users on other homeservers shall continue to exchange messages with each other, and no messages between them shall be lost. | Stop one homeserver and confirm users on the others can still message each other |
| NFR-5 | Security | The system shall implement PQXDH and SPQR as defined in Signal's published specifications. | Conformance checks against the Signal specifications |
| NFR-6 | Performance | An average, non-technical user shall be able to register an account in under 5 minutes, and a message between two online users on the same homeserver shall appear for the recipient within 5 seconds. | Timed registration with test users; timed message delivery between two test accounts |
| NFR-7 | Anonymity | The system shall not require out-of-band credentials (email, phone number, etc.) for registration. | Review of the registration form and API |
| NFR-8 | Usability | The system shall let users start a chat using only the other person's username. | Demonstration of the New conversation flow |
## Additional Documentation
- [Class Reports](https://github.com/JamesRtoP/26-FA26-SP27-SADSL-CYBER/tree/main/Reports)
- [Sprint_1 Report](https://github.com/JamesRtoP/26-FA26-SP27-SADSL-CYBER/blob/main/Sprints/Sprints_1/Sprint_Report/Sprint1_Report.md)
- [Sprint_2 Report](https://github.com/JamesRtoP/26-FA26-SP27-SADSL-CYBER/blob/main/Sprints/Sprints_2/Sprint_Report/Sprint2_Report.md)
- [Testing Guide](TESTING.md)
- [Homeserver Database](Code/homeserver/DATABASE.md)
- [Client Database](Code/Q3_Client/DATABASE.md)
## License
- [GNU General Public License](https://github.com/JamesRtoP/26-FA26-SP27-SADSL-CYBER/blob/main/LICENSE)
