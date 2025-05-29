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
    property string capturedPhotoPath: ""
    property string selectedCategory: ""
    property string processedModelUrl: ""
    property string lastScanId: ""
    property var capturedFrame: null
    property string currentGarmentId: ""
    property string processedPreviewUrl: ""
    property string processedModelKey: ""
    property string processedPreviewKey: ""
    // Generate unique garment ID
    function generateGarmentId() {
        const timestamp = Date.now();
        const randomPart = Math.floor(Math.random() * 1000000); // Increased range

        return `garment_${timestamp}_${randomPart}`;
    }

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
            name: "preview"
            PropertyChanges { target: captureButton; visible: true }
        },
        State {
            name: "categorySelection"
            PropertyChanges { target: categoryDialog; visible: true }
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
                    if(cameraPage.state === "categorySelection") return "Select Category"
                    if(cameraPage.state === "modelPreview") return "Confirm Model"
                    return "Camera Preview"
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

        // Handle scan progress updates
        onScanProgressChanged: function(progress) {
            progressBar.value = progress
        }

        // Handle processed model ready
        onProcessedModelUrlReady: function(modelUrl, previewUrl, modelKey, previewKey) {
            processedModelUrl = modelUrl
            processedPreviewUrl = previewUrl  // Store the preview URL
            processedModelKey = modelKey
            processedPreviewKey = previewKey
            cameraPage.state = "modelPreview"
        }

        // Handle scan processing failures
        onScanProcessingFailed: function(error) {
            errorLabel.text = "Scan processing failed: " + error
            errorLabel.visible = true
            cameraPage.state = "preview"
            progressOverlay.visible = false
        }

        // Handle upload completion
        onUploadCompleted: function(garmentId) {
            console.log("Upload completed for garment:", garmentId)
            stackView.pop()
        }

        // Handle upload failures
        onUploadFailed: function(error) {
            errorLabel.text = "Upload failed: " + error
            errorLabel.visible = true
            cameraPage.state = "preview"
        }

        // Handle upload progress
        onUploadProgressChanged: function(progress) {
            progressBar.value = progress
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
            onImageCaptured: function(requestId, frame) {
                capturedFrame = frame
                cameraPage.state = "categorySelection"
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
            currentGarmentId = generateGarmentId()
            // currentGarmentId = "254858648"
            console.log("Garment id from QT: ", currentGarmentId)
            // Capture image to memory
            imageCapture.capture()
        }

        contentItem: Text {
            text: "Capture"
            font: Style.buttonFont
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    // Category Selection Dialog
    Dialog {
        id: categoryDialog
        title: "Select Garment Category"
        anchors.centerIn: parent
        modal: true
        visible: false

        onAccepted: {
            if (selectedCategory === "") {
                errorLabel.text = "Please select a category"
                errorLabel.visible = true
                return
            }

            // Set category in QMLManager
            qmlManager.setScanCategory(selectedCategory)

            if (capturedFrame) {
                // Pass the captured frame to QMLManager for processing
                qmlManager.handleCapturedFrame(capturedFrame, currentGarmentId)
                capturedFrame = null // Clear after sending
                cameraPage.state = "capturing"
            } else {
                errorLabel.text = "No captured frame available"
                errorLabel.visible = true
                cameraPage.state = "preview"
            }
        }

        onRejected: {
            // Reset to preview state if dialog is cancelled
            cameraPage.state = "preview"
            capturedFrame = null
        }

        ColumnLayout {
            spacing: 20

            ButtonGroup {
                id: categoryGroup
            }

            RadioButton {
                text: "Shirt"
                checked: true
                ButtonGroup.group: categoryGroup
                onCheckedChanged: if(checked) selectedCategory = "shirt"
            }
            RadioButton {
                text: "Pants"
                ButtonGroup.group: categoryGroup
                onCheckedChanged: if(checked) selectedCategory = "pants"
            }
            RadioButton {
                text: "Dress"
                ButtonGroup.group: categoryGroup
                onCheckedChanged: if(checked) selectedCategory = "dress"
            }
            RadioButton {
                text: "Shoes"
                ButtonGroup.group: categoryGroup
                onCheckedChanged: if(checked) selectedCategory = "shoes"
            }

            RowLayout {
                spacing: 10
                Button {
                    text: "Cancel"
                    onClicked: categoryDialog.reject()
                }
                Button {
                    text: "Confirm"
                    onClicked: categoryDialog.accept()
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
            }
        }
    }

    // Model Preview and Confirmation
    Rectangle {
        id: modelPreview
        anchors.fill: parent
        visible: cameraPage.state === "modelPreview"
        color: "#f0f0f0"

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 20
            width: parent.width * 0.8

            Label {
                text: "3D Model Preview"
                font.bold: true
                font.pixelSize: 18
                Layout.alignment: Qt.AlignCenter
            }

            // 3D Preview Component with actual image
            Rectangle {
                Layout.preferredWidth: 300
                Layout.preferredHeight: 300
                Layout.alignment: Qt.AlignCenter
                color: "#e0e0e0"
                border.color: "#ccc"
                border.width: 1
                radius: 5

                Image {
                    id: previewImage
                    anchors.fill: parent
                    anchors.margins: 2
                    source: processedPreviewUrl
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    cache: false
                    visible: processedPreviewUrl !== ""

                    // Loading indicator
                    BusyIndicator {
                        anchors.centerIn: parent
                        running: previewImage.status === Image.Loading
                        width: 50
                        height: 50
                        visible: processedPreviewUrl !== "" && previewImage.status === Image.Loading
                    }

                    // Error handling for image loading
                    onStatusChanged: {
                        if (status === Image.Error) {
                            errorLabel.text = "Failed to load preview image"
                            errorLabel.visible = true
                        }
                    }
                }

                // Fallback placeholder when no image is available
                Label {
                    anchors.centerIn: parent
                    text: processedPreviewUrl === "" ?
                          "Generating preview...\nPlease wait" :
                          "Preview Image\n" + processedModelUrl
                    horizontalAlignment: Text.AlignHCenter
                    color: "#666"
                    visible: processedPreviewUrl === "" || previewImage.status === Image.Error
                    font.pixelSize: 12
                }

                // Zoom/interaction overlay (optional)
                MouseArea {
                    anchors.fill: parent
                    enabled: previewImage.visible && previewImage.status === Image.Ready
                    onClicked: {
                        // Optional: Show full-screen preview
                        fullScreenPreview.visible = true
                    }
                }
            }

            TextField {
                id: garmentName
                placeholderText: "Enter garment name"
                Layout.fillWidth: true
                Layout.preferredHeight: 40
            }

            RowLayout {
                spacing: 20
                Layout.alignment: Qt.AlignCenter

                Button {
                    text: "Save Garment"
                    enabled: garmentName.text.trim() !== "" && processedPreviewUrl !== ""
                    onClicked: {
                        if (garmentName.text.trim() === "") {
                            errorLabel.text = "Please enter a garment name"
                            errorLabel.visible = true
                            return
                        }

                        if (processedPreviewUrl === "") {
                            errorLabel.text = "Preview image not ready. Please wait..."
                            errorLabel.visible = true
                            return
                        }

                        // Call saveGarment with correct parameters: garmentId, name, previewUrl, modelUrl
                        qmlManager.saveGarment(
                            currentGarmentId,           // garmentId (generated in QML)
                            garmentName.text.trim(),    // name
                            processedModelUrl,          // modelUrl
                            processedPreviewUrl,        // previewUrl
                            processedModelKey,
                            processedPreviewKey,
                            category
                        )

                        // Show processing state
                        cameraPage.state = "initial"
                    }
                }

                Button {
                    text: "Retake"
                    onClicked: {
                        // Reset state and restart camera
                        processedModelUrl = ""
                        processedPreviewUrl = ""
                        lastScanId = ""
                        currentGarmentId = ""
                        selectedCategory = ""
                        garmentName.text = ""
                        cameraPage.state = "initial"
                        camera.start()
                    }
                }
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
        // Initialize selected category
        selectedCategory = "shirt"

        // Start camera if not already started
        if (camera.cameraStatus === Camera.UnloadedStatus) {
            if (qmlManager.hasCameraPermission()) {
                camera.start()
            }
        }
    }

    Component.onDestruction: {
        if (camera.cameraStatus === Camera.ActiveStatus) {
            camera.stop()
        }
    }
}
