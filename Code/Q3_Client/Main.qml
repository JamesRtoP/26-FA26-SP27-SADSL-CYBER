import QtQuick // provides basic QML types and UI functionality
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 300
    visible: true
    title: "Q3-Client"

    Column { // column arranges everything inside vertically
        anchors.centerIn: parent // puts whole group in middle of window
        spacing: 10

        TextField {
            id: usernameField
            placeholderText: "Username"
        }

        TextField {
            id: passwordField
            placeholderText: "Password"
            echoMode: TextInput.Password // hide chars on screen
        }

        Button {
            text: "Register"

            // run registerUser() in clientservice.cpp
            onClicked: {
                clientService.registerUser(
                    usernameField.text,
                    passwordField.text
                )
            }
        }

        Button {
            text: "Login"

            onClicked: {
                clientService.loginUser(
                    usernameField.text,
                    passwordField.text
                )
            }
        }
    }
}