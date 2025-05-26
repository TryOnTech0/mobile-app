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

    states: [
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
        onScanProgressChanged: {
            progressBar.value = scanProgress
            if(scanProgress >= 100) {
                processedModelUrl = qmlManager.getProcessedModel()
                cameraPage.state = "modelPreview"
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
                if (!PermissionHelper.checkCameraPermission()) {
                    PermissionHelper.requestCameraPermission()
                } else {
                    startCamera()
                }
            }

            // onCameraStatusChanged: {
            //     if(cameraStatus === Camera.ActiveStatus) {
            //         captureButton.visible = true
            //     }
            // }

            onErrorOccurred: (error, errorString) => {
                errorLabel.text = "Camera Error: " + errorString
                errorLabel.visible = true
                camera.stop()
            }
        }
        imageCapture: ImageCapture {
            id: imageCapture
            onImageSaved: (id, path) => {
                capturedPhotoPath = path
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
            const picturesPath = StandardPaths.writableLocation(StandardPaths.PicturesLocation)
            imageCapture.captureToFile(picturesPath + "/scan_photo.jpg")
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

    // Category Selection Dialog
    Dialog {
        id: categoryDialog
        title: "Select Garment Category"
        anchors.centerIn: parent
        modal: true
        visible: false

        ColumnLayout {
            spacing: 20
            RadioButton {
                text: "Shirt"
                checked: true
                onCheckedChanged: if(checked) selectedCategory = "shirt"
            }
            RadioButton {
                text: "Pants"
                onCheckedChanged: if(checked) selectedCategory = "pants"
            }
            RadioButton {
                text: "Dress"
                onCheckedChanged: if(checked) selectedCategory = "dress"
            }
            RadioButton {
                text: "Shoes"
                onCheckedChanged: if(checked) selectedCategory = "shoes"
            }
            Button {
                text: "Confirm"
                onClicked: {
                    qmlManager.uploadScan(capturedPhotoPath, selectedCategory)
                    cameraPage.state = "capturing"
                    categoryDialog.close()
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
                running: true
                width: 100
                height: 100
            }

            ProgressBar {
                id: progressBar
                width: parent.width * 0.8
                from: 0
                to: 100
                value: 0
            }

            Label {
                text: "Processing scan...\nThis may take a few seconds"
                color: "white"
                horizontalAlignment: Text.AlignHCenter
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

            // 3D Preview Component (Replace with actual 3D viewer)
            Rectangle {
                width: 300
                height: 300
                color: "white"
                border.color: "gray"

                Text {
                    anchors.centerIn: parent
                    text: "3D Model Preview\n(Placeholder)"
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            TextField {
                id: garmentName
                placeholderText: "Enter garment name"
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 20
                Button {
                    text: "Confirm"
                    onClicked: {
                        qmlManager.saveGarment(garmentName.text, selectedCategory, processedModelUrl)
                        stackView.pop()
                    }
                }
                Button {
                    text: "Retake"
                    onClicked: {
                        cameraPage.state = "preview"
                        camera.start()
                    }
                }
            }
        }
    }

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
        function onUploadProgress(percent) {
            progressBar.value = percent
        }
        function onProcessingError(error) {
            errorLabel.text = "Processing error: " + error
            errorLabel.visible = true
            cameraPage.state = "preview"
        }
    }
}
