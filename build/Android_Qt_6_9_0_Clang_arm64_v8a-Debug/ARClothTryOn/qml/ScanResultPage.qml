import QtQuick
import QtQuick.Controls
import ARClothTryOn 1.0

Page {
    id: scanResultPage
    property string previewPath

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20
        Image {
            source: previewPath
            width: 300
            height: 300
        }
        Rectangle {
            width: 300
            height: 300
            color: "lightgray"
            Text { text: "3D Preview"; anchors.centerIn: parent }
        }
        
        RowLayout {
            spacing: 20
            Button {
                text: "Retry"
                onClicked: stackView.pop()
            }
            
            Button {
                text: "Save"
                onClicked: {
                     onClicked: qmlManager.saveScan()
                }
            }
        }
    }
}
