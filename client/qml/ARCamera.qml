import QtQuick
import QtQuick.Controls
import QtMultimedia
import QtQuick.Layouts
import QtCore
import ARClothTryOn 1.0

Page {
    id: cameraPage
    property string garmentId
    property alias manager: qmlManager
    property bool hasCameraPermission: false
    property bool arActive: false

    // Fixed CameraFrameProcessor with proper parameter declarations
    CameraFrameProcessor {
        id: frameProcessor
        videoSink: videoOutput.videoSink  // Connect to the display output

        onArActiveChanged: cameraPage.arActive = arActive
        onArErrorOccurred: function(error) { showError(error) }
        onProcessedFrameReady: function(frame) {
            console.log("Received processed frame")
        }
        onCameraReadyChanged: function(ready) {
            console.log("Camera ready state changed:", ready)
            if (!ready) {
                arActive = false
            }
        }
    }
    header: ToolBar {
        background: Rectangle {
            color: Style.primaryColor
        }
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
                text: arActive ? "AR Session Active" : "Camera Preview"
                color: "white"
                font.bold: true
                font.pixelSize: Style.buttonFont.pixelSize
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
            }
        }
    }

    CaptureSession {
        id: captureSession
        camera: Camera {
            id: camera
            cameraDevice: MediaDevices.defaultVideoInput
            focusMode: Camera.FocusModeAuto

            Component.onCompleted: {
                if (!qmlManager.hasCameraPermission()) {
                    qmlManager.requestCameraPermission()
                } else {
                    startCamera()
                }
            }

            onErrorOccurred: function(error, errorString) {
                showError("Camera Error: " + errorString)
                camera.stop()
            }
        }
        videoOutput: videoOutput
    }

    // Single VideoOutput that shows either camera or processed frames
    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop

        Component.onCompleted: {
            // Connect camera to frame processor for status monitoring
            frameProcessor.monitorCameraStatus(camera)
        }
    }

    // Error label
    Label {
        id: errorLabel
        visible: false
        color: "red"
        font.bold: true
        Layout.alignment: Qt.AlignCenter
        anchors.centerIn: parent

        Timer {
            id: errorTimer
            interval: 5000
            onTriggered: errorLabel.visible = false
        }
    }

    // Permission request overlay
    Rectangle {
        id: permissionOverlay
        anchors.fill: parent
        color: "#80000000"
        visible: !hasCameraPermission
        z: 999

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 20
            width: Math.min(parent.width * 0.8, 400)

            Label {
                text: "Camera Permission Required"
                color: "white"
                font: Style.buttonFont
                Layout.alignment: Qt.AlignCenter
            }

            Label {
                text: "This app needs camera access to provide AR functionality."
                color: "white"
                font: Style.inputFont
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignCenter
            }

            Button {
                text: "Grant Permission"
                Layout.alignment: Qt.AlignCenter
                width: Style.buttonWidth
                height: Style.buttonHeight
                font: Style.buttonFont

                background: Rectangle {
                    radius: height/2
                    color: parent.down ? Qt.darker(Style.primaryColor, 1.2) : Style.primaryColor
                }

                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: qmlManager.requestCameraPermission()
            }
        }
    }

    // Controls overlay
    Rectangle {
        id: controls
        width: parent.width
        height: 100
        anchors.bottom: parent.bottom
        color: "#80000000"
        visible: hasCameraPermission && camera.cameraStatus === Camera.ActiveStatus

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 10

            Button {
                id: startButton
                text: arActive ? "Stop AR" : "Start AR"
                Layout.alignment: Qt.AlignCenter
                width: Style.buttonWidth
                height: Style.buttonHeight

                background: Rectangle {
                    radius: height/2
                    color: parent.down ? Qt.darker(arActive ? "#ff6b6b" : "#4ecdc4", 1.2) :
                           (arActive ? "#ff6b6b" : "#4ecdc4")
                }

                contentItem: Text {
                    text: parent.text
                    font: Style.buttonFont
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (arActive) {
                        stopArProcessing()
                    } else {
                        startArProcessing()
                    }
                }
            }

            Label {
                id: statusText
                Layout.alignment: Qt.AlignCenter
                text: {
                    if (!hasCameraPermission) return "Camera permission required"
                    if (camera.cameraStatus !== Camera.ActiveStatus) return "Camera not ready"
                    return arActive ? "AR Session Active" : "Ready for AR"
                }
                color: "white"
                font.pixelSize: 12
            }
        }
    }

    // Status indicator
    Rectangle {
        width: 20
        height: 20
        radius: 10
        anchors {
            top: parent.top
            right: parent.right
            margins: 20
        }
        color: {
            if (!hasCameraPermission) return "orange"
            if (camera.cameraStatus !== Camera.ActiveStatus) return "orange"
            return arActive ? "green" : "red"
        }
        z: 1001
    }

    // Helper function to start camera
    function startCamera() {
        if (hasCameraPermission &&
            (camera.cameraStatus === Camera.UnloadedStatus ||
             camera.cameraStatus === Camera.StoppedStatus)) {
            camera.start()
        }
    }

    // Helper function to stop camera safely
    function stopCamera() {
        if (camera.cameraStatus === Camera.ActiveStatus) {
            camera.stop()
        }
    }

    // Start AR processing
    function startArProcessing() {
        // Replace with your actual server URL
        const serverUrl = "wss://40e5-193-140-134-140.ngrok-free.app"
        frameProcessor.startArProcessing(serverUrl)
    }

    // Stop AR processing
    function stopArProcessing() {
        frameProcessor.stopArProcessing()
    }

    // Show error message
    function showError(message) {
        errorLabel.text = message
        errorLabel.visible = true
        errorTimer.restart()
    }

    // QMLManager for handling permissions
    QMLManager {
        id: qmlManager

        onPermissionGranted: {
            console.log("Camera permission granted")
            hasCameraPermission = true
            startCamera()
        }

        onPermissionDenied: {
            console.log("Camera permission denied")
            hasCameraPermission = false
            showError("Camera permission denied. Please enable camera access in settings.")
        }
    }

    Component.onCompleted: {
        // Initialize camera permission state
        hasCameraPermission = qmlManager.hasCameraPermission()
        console.log("Camera initialized, has permission:", hasCameraPermission)
    }

    Component.onDestruction: {
        console.log("Camera component being destroyed, cleaning up...")
        stopArProcessing()
        stopCamera()
    }
}
