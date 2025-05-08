import QtQuick
import QtQuick.Controls
import QtMultimedia
import ARClothTryOn 1.0

Page {
    id: cameraPage
    property alias manager: qmlManager

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
