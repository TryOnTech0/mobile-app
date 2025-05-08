import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ARClothTryOn 1.0

Page {
    id: garmentSelectionPage
    background: Rectangle { color: Style.backgroundColor }

    property string selectedGarmentId: ""

    header: ToolBar {
        background: Rectangle { color: Style.primaryColor }
        
        Label {
            anchors.centerIn: parent
            text: "Select Garment"
            color: "white"
            font.bold: true
            font.pixelSize: Style.buttonFont.pointSize * 1.2
        }
    }

    QMLManager {
        id: qmlManager
        onGarmentsChanged: {
            gridView.model = garments
            loadingIndicator.visible = false
        }
    }

    // Main content
    GridView {
        id: gridView
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: Style.gridSpacing
        }
        cellWidth: width / 3 - Style.gridSpacing
        cellHeight: cellWidth + 40 // Extra space for text
        
        delegate: Item {
            width: gridView.cellWidth
            height: gridView.cellHeight
            
            ColumnLayout {
                spacing: Style.gridSpacing
                anchors.fill: parent
                anchors.margins: Style.gridSpacing/2

                // Garment Preview Frame
                Rectangle {
                    id: previewFrame
                    Layout.fillWidth: true
                    Layout.preferredHeight: gridView.cellWidth - Style.gridSpacing
                    radius: 8
                    color: Style.backgroundColor
                    border.color: Style.secondaryColor
                    border.width: 2

                    Image {
                        anchors {
                            fill: parent
                            margins: 2
                        }
                        source: modelData.previewUrl || "qrc:/images/placeholder.png"
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        
                        Rectangle {
                            anchors.fill: parent
                            color: "#80000000"
                            visible: !modelData.isAvailable
                            
                            Text {
                                anchors.centerIn: parent
                                text: "Unavailable"
                                color: "white"
                                font.bold: true
                            }
                        }
                    }

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: parent.status === Image.Loading
                    }
                }

                // Garment Name
                Text {
                    text: modelData.name || "Unnamed Garment"
                    color: Style.primaryColor
                    font: Style.buttonFont
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if(modelData.isAvailable) {
                        selectedGarmentId = modelData.id
                        stackView.push("GarmentPreviewPage.qml", {
                            garmentId: modelData.id,
                            previewImage: modelData.previewUrl
                        })
                    }
                }
            }
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
            width: 8
        }
    }

    // Loading Indicator
    BusyIndicator {
        id: loadingIndicator
        anchors.centerIn: parent
        running: visible
        visible: false
        width: Style.buttonHeight * 2
        height: width
    }

    // Empty State
    Label {
        anchors.centerIn: parent
        text: "No garments found\nScan a new one or check your storage"
        visible: gridView.count === 0 && !loadingIndicator.visible
        horizontalAlignment: Text.AlignHCenter
        color: Style.primaryColor
        font: Style.buttonFont
    }

    Component.onCompleted: {
        if(gridView.count === 0) {
            loadingIndicator.visible = true
            qmlManager.loadGarments()
        }
    }

    // Connection for error handling
    Connections {
        target: qmlManager
        function onGarmentsChanged() {
            if(gridView.count === 0) {
                loadingIndicator.visible = false
            }
        }
    }
}