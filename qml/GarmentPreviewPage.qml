import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ARClothTryOn 1.0

Page {
    id: garmentPreviewPage
    QMLManager {
        id: qmlManager
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20
        
        Rectangle {
            width: 300
            height: 300
            color: "lightgray"
            Text { text: "3D Preview"; anchors.centerIn: parent }
        }
        
        RowLayout {
            spacing: 20
            Button {
                text: "Go Back"
                onClicked: stackView.pop()
            }
            
            Button {
                text: "Try On"
                    onClicked: {
                    qmlManager.tryOnGarment(garmentId)
                    stackView.push("CameraPage.qml")
                }
            }
        }
    }
}
