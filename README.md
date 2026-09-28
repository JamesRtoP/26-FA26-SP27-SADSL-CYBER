# 26-FA26-SP27-SADSL-CYBER
## Project summary
### One-sentence description of the project
Our project is an end-to-end encrypted chat application utilizing post-quantum security to ensure security once quantum computer has become widespread. Additionally our application will feature decentralized servers.
### Additional information about the project
The primary objective of our project is to produce an end-to-end encrypted chat application that uses post-quantum cryptography. Post-quantum cryptography enables our application to remain secure both today and when quantum computing has broken standard encryption schemes. Security and anonymity are paramount to our application. For our application to attract potential users, it must be auditable and anonymously accessible. 

The chat application requires a server for hosting. This server will be responsible for handling user registration and initiating connections. Once the server has initiated a connection, its sole responsibility will be forwarding messages between users and storing message history. The message history exists solely to backup messages for clients changing devices. At no point will the server be able to decrypt stored messages. 

The client-side application will be responsible for all message decryptions. It will be the sole storage location for the encryption keys. To ensure security, device management and device switching will be handled entirely by the client. 
## Installation
### Prerequisites
QT,
libsignal
### Installation Steps
Expand Upon Development

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
Reports: https://github.com/JamesRtoP/26-FA26-SP27-SADSL-CYBER/tree/feat/elastic-dashboard/Reports
