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

        RowLayout {
            anchors {
                fill: parent
                topMargin: 5  // This creates 5px padding at top
            }
            spacing: 10

            // Back Button
            Button {
                text: "← Back"
                font: Style.buttonFont
                palette.buttonText: "white"
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                background: Rectangle {
                    color: "transparent"
                }

                onClicked: {
                    stackView.pop() // Navigate back
                }
            }

            // Title
            Label {
                text: "Scanning..."
                color: "white"
                font.bold: true
                font.pixelSize: Style.buttonFont.pointSize * 1.2
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHRight
            }
        }
    }

    QMLManager {
        id: qmlManager
        onScanProgressChanged: progressBar.value = scanProgress
    }

    CaptureSession {
        camera: Camera {
            id: camera

            focusMode: Camera.FocusModeAutoNear
            customFocusPoint: Qt.point(0.2, 0.2) // Focus relative to top-left corner
        }
        videoOutput: videoOutput
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
    }
    
    Button {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter

        width: Style.buttonWidth
        height: Style.buttonHeight

        background: Rectangle {
            radius: Style.buttonRadius
            color: parent.down ? Qt.darker(Style.primaryColor, 1.2) : Style.primaryColor
        }

        onClicked: {
            progressBar.visible = true
            qmlManager.startScanning()
        }
        contentItem: Text {
            text: "Start Scanning"
            font: Style.buttonFont
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
    
    ProgressBar {
        id: progressBar
        anchors.top: parent.top
        width: parent.width
        visible: false
        indeterminate: true
        from: 0
        to: 100
        value: qmlManager.scanProgress
        
        Connections {
            target: qmlManager
            function onShowPreview(previewPath) {
                stackView.push("ScanResultPage.qml", {"previewPath": previewPath})
            }
        }
    }
}
