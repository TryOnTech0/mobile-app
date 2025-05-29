// qml/AuthorizationPage.qml - Fixed sizing version
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ARClothTryOn 1.0

Page {
    id: authPage

    // Make sure the page fills available space
    anchors.fill: parent // Ensure full parent filling

    background: Rectangle { 
        color: Style.backgroundColor 
        anchors.fill: parent
    }

    property bool isRegisterMode: true

    Component.onCompleted: {
        console.log("AuthorizationPage loaded successfully")
        console.log("Page size:", width, "x", height)
        console.log("Page visible:", visible)
        console.log("Parent size:", parent ? (parent.width + "x" + parent.height) : "no parent")
    }

    NetworkManager {
        id: networkManager
        onRegistrationSucceeded: {
            console.log("Registration succeeded")
            loadingIndicator.running = false
            // Find the StackView by going up the parent chain
            var stackView = authPage.StackView.view
            if (stackView) {
                stackView.push("qrc:/ARClothTryOn/qml/MainPage.qml")
            } else {
                console.log("Could not find StackView")
            }
        }
        onRegistrationFailed: {
            console.log("Registration failed")
            loadingIndicator.running = false
            errorLabel.text = "Registration failed"
            errorLabel.visible = true
        }
        onUserLoggedIn: {
            console.log("User logged in")
            loadingIndicator.running = false
            var stackView = authPage.StackView.view
            if (stackView) {
                stackView.push("qrc:/ARClothTryOn/qml/MainPage.qml")
            }
        }
        onAuthenticationFailed: {
            console.log("Authentication failed")
            loadingIndicator.running = false
            errorLabel.text = "Authentication failed"
            errorLabel.visible = true
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: Style.gridSpacing * 2
        width: Math.min(parent.width * 0.8, 400)

        Label {
            text: isRegisterMode ? qsTr("Create Account") : qsTr("Welcome Back")
            font.pixelSize: 24
            Layout.alignment: Qt.AlignHCenter
            color: Style.primaryTextColor
        }

        // Username Field (Visible only in registration mode)
        TextField {
            id: usernameField
            placeholderText: qsTr("Username")
            visible: isRegisterMode
            font: Style.inputFont
            Layout.fillWidth: true
            Layout.preferredHeight: 40

            background: Rectangle {
                color: "white"
                border.color: "#cccccc"
                radius: 4
            }
        }

        // Email Field
        TextField {
            id: emailField
            placeholderText: qsTr("Email address")
            font: Style.inputFont
            Layout.fillWidth: true
            Layout.preferredHeight: 40

            background: Rectangle {
                color: "white"
                border.color: "#cccccc"
                radius: 4
            }
        }

        // Password Field
        TextField {
            id: passwordField
            placeholderText: qsTr("Password")
            echoMode: TextInput.Password
            font: Style.inputFont
            Layout.fillWidth: true
            Layout.preferredHeight: 40

            background: Rectangle {
                color: "white"
                border.color: "#cccccc"
                radius: 4
            }
        }

        // Error Message
        Label {
            id: errorLabel
            visible: false
            color: Style.errorColor
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        // Submit Button
        Button {
            id: submitButton
            text: isRegisterMode ? qsTr("Register") : qsTr("Log In")
            Layout.preferredWidth: Style.buttonWidth
            Layout.preferredHeight: Style.buttonHeight
            Layout.alignment: Qt.AlignHCenter
            font: Style.buttonFont

            onClicked: {
                console.log("Submit button clicked, mode:", isRegisterMode ? "register" : "login")
                errorLabel.visible = false
                loadingIndicator.running = true

                if(isRegisterMode) {
                    networkManager.registerUser(
                        usernameField.text,
                        emailField.text,
                        passwordField.text
                    )
                } else {
                    networkManager.loginUser(
                        emailField.text,
                        passwordField.text
                    )
                }
            }

            background: Rectangle {
                radius: Style.buttonRadius
                color: parent.down ? Qt.darker(Style.primaryColor, 1.2) : Style.primaryColor
            }

            contentItem: Text {
                text: submitButton.text
                font: submitButton.font
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        // Toggle Mode
        Button {
            text: isRegisterMode ? qsTr("Already have an account? Log In")
                                : qsTr("Need an account? Register")
            flat: true
            Layout.alignment: Qt.AlignHCenter

            onClicked: {
                console.log("Toggle mode clicked")
                isRegisterMode = !isRegisterMode
                errorLabel.visible = false
                usernameField.clear()
                emailField.clear()
                passwordField.clear()
            }

            contentItem: Text {
                text: parent.text
                color: Style.secondaryTextColor
                font.pixelSize: 14
            }

            background: Rectangle {
                color: "transparent"
            }
        }
    }

    // Loading Indicator
    BusyIndicator {
        id: loadingIndicator
        anchors.centerIn: parent
        running: false
        visible: running
        width: Style.buttonHeight * 1.5
        height: width
    }
}
