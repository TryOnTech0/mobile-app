import QtQuick
import QtQuick.Controls
import QtMultimedia
import QtQuick.Layouts
import ARClothTryOn 1.0

Page {
    id: cameraPage
    property alias manager: qmlManager

    header: ToolBar {
        background: Rectangle { color: Style.primaryColor }
        height: 50

        RowLayout {
            anchors.fill: parent
            spacing: 15
            anchors.leftMargin: 10

            Button {
                text: "← Back"
                font: Style.buttonFont
                palette.buttonText: "white"
                flat: true
                onClicked: {
                    camera.stop()
                    stackView.pop()
                }
            }

            Label {
                text: cameraPage.state === "scanning" ? "Scanning..." : "Camera Preview"
                color: "white"
                font.bold: true
                font.pixelSize: Style.buttonFont.pixelSize
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
            }
        }
    }

    QMLManager {
        id: qmlManager
        onScanProgressChanged: {
            progressBar.value = scanProgress
            if(scanProgress >= 100) cameraPage.state = "preview"
        }
    }

    CaptureSession {
            id: captureSession
            camera: Camera {
                id: camera
                cameraDevice: MediaDevices.defaultVideoInput
                focusMode: Camera.FocusModeAuto

                Component.onCompleted: {
                    if (!PermissionHelper.checkCameraPermission()) {
                        PermissionHelper.requestCameraPermission()
                    } else {
                        startCamera()
                    }
                }

                onErrorOccurred: (error, errorString) => {
                    errorLabel.text = "Camera Error: " + errorString
                    errorLabel.visible = true
                    camera.stop()
                }
            }
            videoOutput: videoOutput
        }

        VideoOutput {
            id: videoOutput
            anchors.fill: parent
            fillMode: VideoOutput.PreserveAspectCrop
        }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20

        Label {
            id: errorLabel
            visible: false
            color: "red"
            font.bold: true
            Layout.alignment: Qt.AlignCenter
        }

        ProgressBar {
            id: progressBar
            visible: cameraPage.state === "scanning"
            width: parent.width * 0.8
            from: 0
            to: 100
            indeterminate: value === 0
        }
    }

    Button {
        id: scanButton
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            bottomMargin: 30
        }
        width: Style.buttonWidth
        height: Style.buttonHeight
        enabled: camera.cameraStatus === Camera.ActiveStatus

        background: Rectangle {
            radius: Style.buttonRadius
            color: parent.down ? Qt.darker(Style.primaryColor, 1.2) :
                   parent.enabled ? Style.primaryColor : Qt.darker(Style.primaryColor, 1.5)
        }

        onClicked: {
            cameraPage.state = "scanning"
            qmlManager.startScanning()
        }

        contentItem: Text {
            text: "Start Scanning"
            font: Style.buttonFont
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            opacity: parent.enabled ? 1 : 0.6
        }
    }

    states: [
        State {
            name: "scanning"
            PropertyChanges { target: scanButton; visible: false }
        },
        State {
            name: "preview"
            PropertyChanges { target: scanButton; visible: true }
        }
    ]

    Component.onCompleted: {
        if (camera.cameraStatus === Camera.UnloadedStatus) {
            camera.start()
        }
    }

    Component.onDestruction: {
        if (camera.cameraStatus === Camera.ActiveStatus) {
            camera.stop()
        }
    }

    Connections {
        target: qmlManager
        function onShowPreview(previewPath) {
            camera.stop()
            stackView.push("ScanResultPage.qml", {"previewPath": previewPath})
        }
    }
}
