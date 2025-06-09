import QtQuick
import QtQuick.Controls
import QtMultimedia
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore
import ARClothTryOn 1.0

Page {
    id: cameraPage
    property alias manager: qmlManager
    property string garmentId: ""
    property string processedPreviewUrl: ""
    property string processedModelUrl: ""
    property string lastScanId: ""
    property string selectedCategory: ""
    property string garmentName: ""

    states: [
        State {
            name: "initial"
            PropertyChanges { target: captureButton; visible: true }
            PropertyChanges { target: progressOverlay; visible: false }
        },
        State {
            name: "capturing"
            PropertyChanges { target: captureButton; visible: false }
            PropertyChanges { target: progressOverlay; visible: true }
        },
        State {
            name: "modelPreview"
        }
    ]

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
                text: {
                    if(cameraPage.state === "capturing") return "Processing..."
                    if(cameraPage.state === "modelPreview") return "Model Preview"
                    return "AR Session"
                }
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
    }

    ImageProcessor {
        id: imageProcessor
        serverUrl: "https://c449-193-140-134-140.ngrok-free.app" // HTTP endpoint instead of WebSocket

        onProcessedImageReceived: function(imageData) {
            // Create base64 data URL for display
            processedPreviewUrl = "data:image/jpeg;base64," + Qt.btoa(imageData)
            cameraPage.state = "modelPreview"
        }

        onProcessingProgress: function(progress) {
            progressBar.value = progress * 100
        }

        onProcessingError: function(errorMessage) {
            errorLabel.text = errorMessage
            errorLabel.visible = true
            cameraPage.state = "initial"
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
                errorLabel.text = "Camera Error: " + errorString
                errorLabel.visible = true
                camera.stop()
            }
        }

        imageCapture: ImageCapture {
            id: imageCapture

            onImageCaptured: function(requestId, preview) {
                // Convert QImage to JPEG byte array and process
                // The preview parameter is a QImage that we can convert
                const jpegData = qmlManager.convertImageToJpeg(preview)
                imageProcessor.handleCapturedImage(jpegData, garmentId)
            }

            onErrorOccurred: function(requestId, error, message) {
                errorLabel.text = "Capture Error: " + message
                errorLabel.visible = true
                cameraPage.state = "initial"
            }
        }

        videoOutput: videoOutput
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
    }

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

        onVisibleChanged: {
            if (visible) {
                errorTimer.start()
            }
        }
    }

    // Capture Button
    Button {
        id: captureButton
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            bottomMargin: 30
        }
        width: Style.buttonWidth
        height: Style.buttonHeight
        visible: camera.cameraStatus === Camera.ActiveStatus

        background: Rectangle {
            radius: height/2
            color: parent.down ? Qt.darker("red", 1.2) : "red"
        }

        onClicked: {
            // Generate new garment ID for this capture
            console.log("Garment id:", garmentId)

            // Capture image (will trigger onImageCaptured)
            imageCapture.capture()
            cameraPage.state = "capturing"
        }

        contentItem: Text {
            text: "Capture"
            font: Style.buttonFont
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    // Model Preview
    Rectangle {
        id: modelPreview
        anchors.fill: parent
        visible: cameraPage.state === "modelPreview"
        color: "#f0f0f0"

        ColumnLayout {
            anchors.fill: parent
            spacing: 20
            anchors.margins: 20

            Label {
                text: "Virtual Try-On Result"
                font.bold: true
                font.pixelSize: 24
                Layout.alignment: Qt.AlignHCenter
            }

            // Preview Image
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignCenter
                color: "#e0e0e0"
                border.color: "#ccc"
                border.width: 1
                radius: 5

                Image {
                    id: previewImage
                    anchors.fill: parent
                    anchors.margins: 10
                    source: processedPreviewUrl
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    cache: false

                    // Loading indicator
                    BusyIndicator {
                        anchors.centerIn: parent
                        running: previewImage.status === Image.Loading
                        width: 50
                        height: 50
                        visible: running
                    }
                }

                // Fallback if image fails to load
                Label {
                    anchors.centerIn: parent
                    text: "Preview Image\n" + (previewImage.status === Image.Error ?
                          "Failed to load image" : "Processing...")
                    horizontalAlignment: Text.AlignHCenter
                    color: "#666"
                    visible: processedPreviewUrl === "" || previewImage.status === Image.Error
                    font.pixelSize: 16
                }
            }

            // Action Buttons
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 20

                Button {
                    text: "Retake"
                    font: Style.buttonFont
                    onClicked: {
                        processedPreviewUrl = ""
                        processedModelUrl = ""
                        garmentId = ""
                        cameraPage.state = "initial"
                        camera.start()
                    }
                }

                Button {
                    text: "Save Result"
                    font: Style.buttonFont
                    palette.button: "green"
                    onClicked: {
                        // Implementation for saving the result
                        qmlManager.saveImageResult(processedPreviewUrl)
                        stackView.pop()
                    }
                }
            }
        }
    }

    // Processing Overlay
    Rectangle {
        id: progressOverlay
        anchors.fill: parent
        color: "#80000000"
        visible: false

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 20

            BusyIndicator {
                Layout.alignment: Qt.AlignCenter
                running: progressOverlay.visible
                width: 100
                height: 100
            }

            ProgressBar {
                id: progressBar
                Layout.preferredWidth: 250
                Layout.alignment: Qt.AlignCenter
                from: 0
                to: 100
                value: 0
            }

            Label {
                text: "Processing scan...\nThis may take a few seconds"
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignCenter
                font.pixelSize: 16
            }
        }
    }

    // Handle permission results
    Connections {
        target: qmlManager

        function onPermissionGranted() {
            camera.start()
        }

        function onPermissionDenied() {
            errorLabel.text = "Camera permission denied. Please enable camera access in settings."
            errorLabel.visible = true
        }
    }

    Component.onCompleted: {
        if (camera.cameraStatus === Camera.UnloadedStatus && qmlManager.hasCameraPermission()) {
            camera.start()
        }
    }

    Component.onDestruction: {
        if (camera.cameraStatus === Camera.ActiveStatus) {
            camera.stop()
        }
    }
}
