import QtQuick // provides basic QML types and UI functionality
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 300
    visible: true
    title: "Q3-Client"
    property bool loggedIn: false
    property bool chatActive: false
    property bool chatRequestPending: false

    Column { // column arranges everything inside vertically
        anchors.centerIn: parent // puts whole group in middle of window
        spacing: 10
        visible: !loggedIn

        TextField {
            id: usernameField
            placeholderText: "Username"
        }

        TextField {
            id: passwordField
            placeholderText: "Password"
            echoMode: TextInput.Password // hide chars on screen
        }

        Text {
            id: registrationError
            color: "red"
            text: ""
            visible: text !== "" // only display if text is present
        }
        Text {
            id: loginError
            color: "red"
            text: ""
            visible: text !== ""
        }

        Connections { // allows QML to listen to signals from cpp file
            target: clientService // object to listen to

            function onRegistrationFailed(message){ // runs when cpp emits registrationFailed signal
                registrationError.text = message;
            }
            function onRegistrationSucceeded() {
                registrationError.text = ""; // wipe away any red-error messages
            }
            function onLoginFailed(message) {
                loginError.text = message;
            }
            function onLoginSucceeded() {
                loginError.text = "";
                loggedIn = true;
            }
            function onChatFailed(message) {
                chatError.text = message;
                chatSuccess.text = "";
                chatActive = false;
                chatRequestPending = false;
            }
            function onChatStarted() {
                chatError.text = "";
                chatSuccess.text = "Chat started successfully!";
                chatActive = true;
                chatRequestPending = false;
            }
            function onMessageFailed(message) {
                messageError.text = message;
            }
            function onMessageSent() {
                messageError.text = "";
                messageField.text = "";
            }
        }

        Button {
            text: "Register"
            enabled: usernameField.text.trim() !== "" &&
                     passwordField.text.trim() !== ""
            // run registerUser() in clientservice.cpp
            onClicked: {
                loginError.text = "";
                clientService.registerUser(
                    usernameField.text,
                    passwordField.text
                )
            }
        }

        Button {
            text: "Login"
            enabled: usernameField.text.trim() !== "" &&
                     passwordField.text.trim() !== ""
            onClicked: {
                registrationError.text = "";
                clientService.loginUser(
                    usernameField.text,
                    passwordField.text
                )
            }
        }
    }
    Column {
        anchors.centerIn: parent
        spacing: 10
        visible: loggedIn

        TextField {
            id: recipientField
            placeholderText: "Recipient Username"
            enabled: !chatRequestPending
            onTextChanged: {
                chatActive = false;
                chatSuccess.text = "";
                chatError.text = "";
            }
        }

        Button {
            text: "Start Chat"
            enabled: recipientField.text.trim() !== "" && !chatRequestPending

            onClicked: {
                chatRequestPending = true;
                clientService.startChat(recipientField.text);
            }
        }

        Text {
            id: chatError
            color: "red"
            text: ""
            visible: text !== ""
        }

        Text {
            id: chatSuccess
            color: "green"
            text: ""
            visible: text !== ""
        }

        TextField {
            id: messageField
            placeholderText: "Type a message..."
        }

        Button {
            text: "Send Message"
            enabled: chatActive && messageField.text.trim() !== ""
                    && recipientField.text.trim() !== "" &&
                    !chatRequestPending

            onClicked: {
                clientService.sendMessage(recipientField.text, messageField.text);
            }
        }

        Text {
            id: messageError
            color: "red"
            text: ""
            visible: text !== ""
        }
    }
}