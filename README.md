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
TODO: List what a user needs to have installed before running the installation
instructions below (e.g., git, which versions of Ruby/Rails)
### Installation Steps
TODO: Describe the installation process (making sure you mention `bundle install`).
Instructions need to be such that a user can just copy/paste the commands to get
things set up and running.

# Requirements

## Homeserver Requirements

### FR-01: REST API Endpoints
**Description:** Homeservers shall provide a REST API with POST endpoints for a client's public keys and GET endpoints for retrieving a client's messages.  
**Source:** Team technical decision  
**Priority:** Level 0

### FR-02: User Key Retrieval
**Description:** Homeservers shall retrieve user public keys from other homeservers and provide them to requesting clients for initial key exchange.  
**Source:** Team technical decision  
**Priority:** Level 0

### FR-03: New Key Generation
**Description:** Homeservers shall retrieve new keys from a client application when the majority of that user's public keys have been used.  
**Source:** Team technical decision  
**Priority:** Level 1

## Client Application Requirements

### FR-04: Homeserver Selection
**Description:** Client applications shall select their homeserver during user registration.  
**Source:** Client Project Specification  
**Priority:** Level 1

### FR-05: Request User Keys
**Description:** Client applications shall request keys for a specific client from their homeserver before sending them any events.  
**Source:** Team technical decision  
**Priority:** Level 1

### FR-06: Event Generation
**Description:** Client applications will generate events for starting a conversation, sending a message during a conversation, and deleting a message. These events are what will be sent to other clients.  
**Source:** Team technical decision  
**Priority:** Level 1

### FR-07: End-to-End Encryption
**Description:** Client applications shall encrypt all events before sending them to the homeserver and decrypt all their own messages from their homeserver.  
**Source:** Client Project Specification  
**Priority:** Level 1

## Cryptography Requirements

### FR-08: Initial Key Exchange
**Description:** Initial key exchange between clients will utilize Post-Quantum Extended Diffie-Hellman (PQXDH).  
**Source:** Team technical decision  
**Priority:** Level 0

### FR-09: Message Encryption
**Description:** Per-message encryption will utilize Sparse Post-Quantum Ratchet (SPQR).  
**Source:** Team technical decision
**Priority:** Level 0
## Known Problems
TODO: Describe any known issues, bugs, odd behaviors or code smells.
Provide steps to reproduce the problem and/or name a file or a function where the
problem lives.
## Additional Documentation
TODO: Provide links to additional documentation that may exist in the repo, e.g.,
* Sprint reports
* User links
