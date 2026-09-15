Update your `README.md` using the template below. We are specifically interested in
the installation instructions (e.g., all the gems, how to load real or seed data,
etc.). This sample README was developed for a Rails project, so you can swap the
"Gems" section of the "Installation" instructions to include add-ons that are
relevant to you.
If any of the sections in this template grows to more than one screen, consider
placing it in its own file and linking to it from this file. Those files could live
in a subdirectory called `docs`.
**Make sure to check out the repo anew and test your installation instructions.**
Provide a README file with the following information:

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
## Functionality
TODO: Write usage instructions. Structuring it as a walkthrough can help structure
this section,
and showcase your features.
## Known Problems
TODO: Describe any known issues, bugs, odd behaviors or code smells.
Provide steps to reproduce the problem and/or name a file or a function where the
problem lives.
## Additional Documentation
TODO: Provide links to additional documentation that may exist in the repo, e.g.,
* Sprint reports
* User links