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
        topPadding: 15
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
                text: "Select Garment"
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
        onGarmentsChanged: {
            console.log("Garments changed, count: " + garments.length)
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
            id: delegateItem
            width: gridView.cellWidth
            height: gridView.cellHeight

            // Add a visual indication for debugging
            Rectangle {
                id: debugHighlight
                anchors.fill: parent
                color: "transparent"
                border.color: "transparent"
                border.width: 2

                // Visual feedback on hover
                states: State {
                    name: "hovered"
                    when: mouseArea.containsMouse
                    PropertyChanges {
                        target: debugHighlight
                        border.color: Style.primaryColor
                    }
                }
            }

            ColumnLayout {
                id: contentLayout
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
                        id: previewImage
                        anchors {
                            fill: parent
                            margins: 2
                        }
                        source: model.modelData.previewUrl
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        sourceSize: Qt.size(200, 200)

                        // Enhanced error handling
                        onStatusChanged: {
                            if (status === Image.Error) {
                                console.log("Error loading image:", source)
                                console.log("Possible SSL/TLS issue - check if OpenSSL is properly configured")
                                errorOverlay.visible = true
                                errorText.text = "Network Error\n(Check SSL config)"
                            } else if (status === Image.Ready) {
                                errorOverlay.visible = false
                            }
                        }

                        Rectangle {
                            id: errorOverlay
                            anchors.fill: parent
                            color: Qt.rgba(0.5, 0.5, 0.5, 0.8)
                            visible: false
                            radius: 8

                            Column {
                                anchors.centerIn: parent
                                spacing: 5

                                Text {
                                    id: errorText
                                    text: "Unavailable"
                                    color: "white"
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                // Retry button
                                Rectangle {
                                    width: 60
                                    height: 25
                                    color: Style.primaryColor
                                    radius: 4

                                    Text {
                                        anchors.centerIn: parent
                                        text: "Retry"
                                        color: "white"
                                        font.pixelSize: 10
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            // Force reload the image
                                            var oldSource = previewImage.source
                                            previewImage.source = ""
                                            previewImage.source = oldSource
                                        }
                                    }
                                }
                            }
                        }
                    }

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: previewImage.status === Image.Loading
                        visible: previewImage.status === Image.Loading
                    }
                }

                // Garment Name
                Text {
                    text: model.modelData.name
                    color: Style.primaryColor
                    font: Style.buttonFont
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true

                onClicked: {
                    console.log("Item clicked: " + model.modelData.id)
                    if (model.modelData.isAvailable) {
                        selectedGarmentId = model.modelData.id  // Access id

                        // Create the preview page component
                        var component = Qt.createComponent("GarmentPreviewPage.qml");
                        if (component.status === Component.Ready) {
                            var page = component.createObject(null, {
                                "garmentId": model.modelData.id,
                                "previewImage": model.modelData.previewUrl,
                                "modelObject": model.modelData.modelwUrl
                            });
                            stackView.push(page);
                        } else if (component.status === Component.Error) {
                            console.error("Error loading component: " + component.errorString());

                            // Fallback: Try direct push with relative path
                            try {
                                stackView.push("GarmentPreviewPage.qml", {
                                    "garmentId": model.modelData.id,
                                    "previewImage": model.modelData.previewUrl,
                                    "modelObject": model.modelData.modelwUrl
                                });
                            } catch (e) {
                                console.error("Fallback navigation failed: " + e);
                            }
                        } else {
                            console.log("Component not ready, status: " + component.status);
                        }
                    }
                }

                onPressed: {
                    debugHighlight.border.color = Style.primaryColor
                    debugHighlight.color = Qt.rgba(Style.primaryColor.r,
                                                 Style.primaryColor.g,
                                                 Style.primaryColor.b, 0.1)
                }

                onReleased: {
                    debugHighlight.border.color = mouseArea.containsMouse ?
                        Style.primaryColor : "transparent"
                    debugHighlight.color = "transparent"
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
        console.log("GarmentSelectionPage completed")
        if(gridView.count === 0) {
            loadingIndicator.visible = true
            qmlManager.fetchGarments()
        }
    }

    // Connection for error handling
    Connections {
        target: qmlManager
        function onGarmentsChanged() {
            console.log("Garments changed signal received")
            if(gridView.count === 0) {
                loadingIndicator.visible = false
            }
        }
    }
}
