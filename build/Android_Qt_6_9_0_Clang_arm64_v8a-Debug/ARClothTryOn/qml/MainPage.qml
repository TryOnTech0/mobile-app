// qml/MainPage.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ARClothTryOn 1.0

Page {
    id: mainPage

    // Fix: Explicit import of Style
    background: Rectangle { color: Style.backgroundColor }

    QMLManager {
        id: qmlManager
        onPermissionGranted: {
            console.log("Camera permission granted")
            permissionDialog.close()
            stackView.push("qrc:/ARClothTryOn/qml/CameraPage.qml")
        }
        onPermissionDenied: {
            console.log("Camera permission denied")
            permissionDialog.close()
            permissionErrorDialog.open()
        }
        // onGarmentsChanged: {
        //     stackView.push("qrc:/ARClothTryOn/qml/GarmentSelectionPage.qml")
        // }
    }
    // Enable console.log output in QML
    Component.onCompleted: {
        qmlManager.initializeApp();
        console.log("Main.qml loaded")
    }
    
    ColumnLayout {
        anchors.centerIn: parent
        spacing: Style.gridSpacing * 2

        // Scan Cloth Button
        Button {
            id: scanButton
            text: qsTr("Scan Cloth")
            Layout.preferredWidth: Style.buttonWidth
            Layout.preferredHeight: Style.buttonHeight
            font: Style.buttonFont

            onClicked: {
                console.log("Scan button clicked")
                if(qmlManager.hasCameraPermission()) {
                    console.log("Camera permission already granted, opening camera")
                    stackView.push("qrc:/ARClothTryOn/qml/CameraPage.qml")
                } else {
                    console.log("Requesting camera permission")
                    permissionDialog.open()
                }
            }

            background: Rectangle {
                radius: Style.buttonRadius
                color: parent.down ? Qt.darker(Style.primaryColor, 1.2) : Style.primaryColor
            }

            contentItem: Text {
                text: scanButton.text
                font: scanButton.font
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        // Select Garment Button
        Button {
            id: selectButton
            text: qsTr("Select Garment")
            Layout.preferredWidth: Style.buttonWidth
            Layout.preferredHeight: Style.buttonHeight
            font: Style.buttonFont

            onClicked: {
                console.log("Select garment button clicked")
                // qmlManager.fetchGarments()
                stackView.push("qrc:/ARClothTryOn/qml/GarmentSelectionPage.qml")
            }

            background: Rectangle {
                radius: Style.buttonRadius
                color: parent.down ? Qt.darker(Style.secondaryColor, 1.2) : Style.secondaryColor
            }

            contentItem: Text {
                text: selectButton.text
                font: selectButton.font
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    // Permission Request Dialog
    Dialog {
        id: permissionDialog
        anchors.centerIn: parent
        title: "Camera Access Required"
        modal: true
        closePolicy: Dialog.CloseOnEscape

        contentItem: Label {
            text: "This app needs camera access to scan garments.\nPlease grant camera permissions."
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        footer: DialogButtonBox {
            Button {
                text: "Cancel"
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
            Button {
                text: "Grant Permission"
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }

        onAccepted: {
            console.log("Permission dialog accepted, requesting camera permission")
            qmlManager.requestCameraPermission()
        }
        onRejected: close()
    }

    // Permission Error Dialog
    Dialog {
        id: permissionErrorDialog
        anchors.centerIn: parent
        title: "Permission Denied"
        modal: true

        contentItem: Label {
            text: "Camera permission is required to use this feature.\nPlease enable it in device settings."
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        footer: DialogButtonBox {
            Button {
                text: "OK"
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
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

    // Connection Handlers
    Connections {
        target: qmlManager

        function onScanProgressChanged(progress) {
            console.log("Scan progress: " + progress)
            if(progress > 0 && progress < 100) {
                loadingIndicator.running = true
            } else {
                loadingIndicator.running = false
            }
        }
    }
}
