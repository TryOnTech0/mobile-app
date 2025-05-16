import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt3D.Core
import Qt3D.Render
import Qt3D.Input
import Qt3D.Extras
import QtQuick.Scene3D 2.0
import ARClothTryOn 1.0

Page {
    id: garmentPreviewPage
    property string garmentId
    property url previewImage

    // State variables for garment manipulation (model only)
    property real scaleValue: 1.0
    property real modelRotationX: 0     // No initial X rotation to see front of garment
    property real modelRotationY: 0     // No initial Y rotation
    property real modelRotationZ: 0     // No initial Z rotation
    property vector2d lastTouchPos: Qt.vector2d(0, 0)
    property bool isDragging: false

    Component.onCompleted: {
        console.log("Style properties:",
            "Primary color:", Style.primaryColor,
            "Background color:", Style.backgroundColor,
            "Button font:", Style.buttonFont
        )
    }

    background: Rectangle { color: Style.backgroundColor }

    QMLManager { id: qmlManager }

    Item {
        id: contentArea
        anchors.fill: parent

        // Zoom Controls (Top-Right)
        Column {
            z: 999  // Ensure buttons stay on top
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 20
            spacing: 15

            Button {
                id: zoomInButton
                width: 50
                height: 50

                contentItem: Item {
                    Text {
                        text: "+"
                        color: "white"
                        font.pixelSize: 28
                        anchors.centerIn: parent
                    }
                }

                background: Rectangle {
                    color: zoomInButton.pressed ? Qt.darker(Style.primaryColor, 1.2) :
                           zoomInButton.hovered ? Qt.lighter(Style.primaryColor, 1.2) :
                           Style.primaryColor
                    radius: width/2
                }

                onPressed: {
                    // Scale the model up
                    scaleValue += 0.1
                    if (scaleValue > 3.0) scaleValue = 3.0
                }
            }

            Button {
                id: zoomOutButton
                width: 50
                height: 50

                contentItem: Item {
                    Text {
                        text: "-"
                        color: "white"
                        font.pixelSize: 28
                        anchors.centerIn: parent
                    }
                }

                background: Rectangle {
                    color: zoomOutButton.pressed ? Qt.darker(Style.primaryColor, 1.2) :
                           zoomOutButton.hovered ? Qt.lighter(Style.primaryColor, 1.2) :
                           Style.primaryColor
                    radius: width/2
                }

                onPressed: {
                    // Scale the model down
                    scaleValue -= 0.1
                    if (scaleValue < 0.5) scaleValue = 0.5
                }
            }
        }

        Scene3D {
            id: scene3d
            anchors.fill: parent
            focus: true
            aspects: ["input", "logic"]
            multisample: true

            Entity {
                id: sceneRoot

                components: [
                    RenderSettings {
                        activeFrameGraph: ForwardRenderer {
                            camera: mainCamera
                            clearColor: "lightgray"  // Scene background stays stable
                        }
                    },
                    InputSettings { }
                ]

                Camera {
                    id: mainCamera
                    // Adjusted camera position to view the front of the garment
                    position: Qt.vector3d(0, 0, 5)  // Camera positioned in front of the model
                    viewCenter: Qt.vector3d(0, 0, 0)
                    upVector: Qt.vector3d(0, 1, 0)  // Ensuring "up" is aligned correctly
                    fieldOfView: 45
                    nearPlane: 0.1
                    farPlane: 1000.0
                }

                Entity {
                    components: DirectionalLight {
                        intensity: 1.0
                        worldDirection: Qt.vector3d(0, -1, 0)
                    }
                }

                Entity {
                    components: DirectionalLight {
                        intensity: 0.4
                        worldDirection: Qt.vector3d(0.5, -0.5, 0.5)
                    }
                }

                Entity {
                    id: modelEntity

                    // Ensure model is centered in the view
                    components: [
                        Mesh {
                            source: "qrc:/garments/" + garmentId.split('_')[0] + "/model.obj"
                        },
                        PhongMaterial {
                            diffuse: "white"
                            ambient: "gray"
                            shininess: 100
                        },
                        Transform {
                            id: garmentTransform
                            // Keep model centered and rotate it around its own axes
                            matrix: {
                                let m = Qt.matrix4x4();

                                // First translate to origin if needed (for models not centered at origin)
                                // m.translate(-modelCenter) would go here if we knew model center

                                // Apply rotations around the model's own axes
                                m.rotate(modelRotationX, Qt.vector3d(1, 0, 0));
                                m.rotate(modelRotationY, Qt.vector3d(0, 1, 0));
                                m.rotate(modelRotationZ, Qt.vector3d(0, 0, 1));

                                // Scale the model
                                m.scale(garmentPreviewPage.scaleValue);

                                // Translate back if needed
                                // m.translate(modelCenter) would go here if we translated earlier

                                return m;
                            }
                        }
                    ]
                }
            }
        }

        // Improved Touch Area for Mobile
        MouseArea {
            id: touchArea
            anchors.fill: parent
            property point previousPos: Qt.point(0, 0)
            property real pinchStartDist: 0
            property bool isPinching: false

            // For debugging touches
            Rectangle {
                id: touchIndicator
                width: 20
                height: 20
                radius: 10
                color: "red"
                opacity: 0.5
                visible: false
            }

            // Handle single touch for rotation around model's own axes
            onPressed: function(mouse) {
                previousPos = Qt.point(mouse.x, mouse.y)
                isDragging = true
                autoRotate.stop()

                // Debug visualization
                touchIndicator.x = mouse.x - touchIndicator.width/2
                touchIndicator.y = mouse.y - touchIndicator.height/2
                touchIndicator.visible = true
            }

            onPositionChanged: function(mouse) {
                if (!isDragging) return

                const dx = mouse.x - previousPos.x
                const dy = mouse.y - previousPos.y

                // Apply rotation to model only around its own axes
                // Reversed direction for more intuitive control
                modelRotationY += dx * 0.5  // Horizontal motion rotates around Y axis
                modelRotationX += dy * 0.5  // Vertical motion rotates around X axis

                previousPos = Qt.point(mouse.x, mouse.y)

                // Update debug visualization
                touchIndicator.x = mouse.x - touchIndicator.width/2
                touchIndicator.y = mouse.y - touchIndicator.height/2
            }

            onReleased: function() {
                isDragging = false
                touchIndicator.visible = false
            }
        }

        // Multi-touch handling with PinchHandler
        PinchHandler {
            id: pinch
            target: null // Don't move any item directly
            minimumScale: 0.5
            maximumScale: 3.0

            onActiveChanged: {
                if (active) {
                    autoRotate.stop()
                }
            }

            onScaleChanged: {
                if (active) {
                    // Apply scale to the model
                    scaleValue = pinch.scale * pinch.previousScale

                    // Keep scale within reasonable bounds
                    if (scaleValue < 0.5) scaleValue = 0.5
                    if (scaleValue > 3.0) scaleValue = 3.0
                }
            }
        }

        // Debug View
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            color: "#80000000"
            width: debugText.width + 20
            height: debugText.height + 10
            visible: false // Set to true for debugging

            Text {
                id: debugText
                anchors.centerIn: parent
                text: `Scale: ${scaleValue.toFixed(2)}, Rotation: (${modelRotationX.toFixed(1)}, ${modelRotationY.toFixed(1)})`
                color: "white"
                font.pixelSize: 12
            }
        }
    }

    // Auto rotation timer
    Timer {
        id: autoRotate
        interval: 16
        running: false
        repeat: true
        onTriggered: modelRotationY += 1.2
    }

    // Improved Control Panel
    Rectangle {
        id: controlPanel
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 30
        width: controlLayout.width + 40
        height: controlLayout.height + 40
        color: Qt.rgba(Style.primaryColor.r, Style.primaryColor.g, Style.primaryColor.b, 0.7)
        radius: 10

        ColumnLayout {
            id: controlLayout
            anchors.centerIn: parent
            spacing: 15

            RowLayout {
                spacing: 15
                Layout.alignment: Qt.AlignHCenter

                // X-axis rotation controls
                Button {
                    id: rotateXUpButton
                    Layout.preferredWidth: 50
                    Layout.preferredHeight: 40

                    contentItem: Text {
                        text: "⬆️"
                        color: "white"
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: rotateXUpButton.pressed ? Qt.darker(Style.primaryColor, 1.2) :
                               rotateXUpButton.hovered ? Qt.lighter(Style.primaryColor, 1.2) :
                               Style.primaryColor
                        radius: 5
                    }

                    onPressed: modelRotationX += 15
                }

                Button {
                    id: rotateXDownButton
                    Layout.preferredWidth: 50
                    Layout.preferredHeight: 40

                    contentItem: Text {
                        text: "⬇️"
                        color: "white"
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: rotateXDownButton.pressed ? Qt.darker(Style.primaryColor, 1.2) :
                               rotateXDownButton.hovered ? Qt.lighter(Style.primaryColor, 1.2) :
                               Style.primaryColor
                        radius: 5
                    }

                    onPressed: modelRotationX -= 15
                }

                // Y-axis rotation controls
                Button {
                    id: rotateYLeftButton
                    Layout.preferredWidth: 50
                    Layout.preferredHeight: 40

                    contentItem: Text {
                        text: "⬅️"
                        color: "white"
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: rotateYLeftButton.pressed ? Qt.darker(Style.primaryColor, 1.2) :
                               rotateYLeftButton.hovered ? Qt.lighter(Style.primaryColor, 1.2) :
                               Style.primaryColor
                        radius: 5
                    }

                    onPressed: modelRotationY -= 15
                }

                Button {
                    id: rotateYRightButton
                    Layout.preferredWidth: 50
                    Layout.preferredHeight: 40

                    contentItem: Text {
                        text: "➡️"
                        color: "white"
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: rotateYRightButton.pressed ? Qt.darker(Style.primaryColor, 1.2) :
                               rotateYRightButton.hovered ? Qt.lighter(Style.primaryColor, 1.2) :
                               Style.primaryColor
                        radius: 5
                    }

                    onPressed: modelRotationY += 15
                }

                // Z-axis rotation controls
                Button {
                    id: rotateZLeftButton
                    Layout.preferredWidth: 50
                    Layout.preferredHeight: 40

                    contentItem: Text {
                        text: "↶"  // Counterclockwise
                        color: "white"
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: rotateZLeftButton.pressed ? Qt.darker(Style.primaryColor, 1.2) :
                               rotateZLeftButton.hovered ? Qt.lighter(Style.primaryColor, 1.2) :
                               Style.primaryColor
                        radius: 5
                    }

                    onPressed: modelRotationZ -= 15
                }

                Button {
                    id: rotateZRightButton
                    Layout.preferredWidth: 50
                    Layout.preferredHeight: 40

                    contentItem: Text {
                        text: "↷"  // Clockwise
                        color: "white"
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: rotateZRightButton.pressed ? Qt.darker(Style.primaryColor, 1.2) :
                               rotateZRightButton.hovered ? Qt.lighter(Style.primaryColor, 1.2) :
                               Style.primaryColor
                        radius: 5
                    }

                    onPressed: modelRotationZ += 15
                }
            }

            RowLayout {
                spacing: 15
                Layout.alignment: Qt.AlignHCenter

                Button {
                    id: resetViewButton
                    Layout.preferredWidth: 110
                    Layout.preferredHeight: 40

                    contentItem: Text {
                        text: "Reset View"
                        color: "white"
                        font.family: Style.buttonFont.family
                        font.pixelSize: Style.buttonFont.pixelSize
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: resetViewButton.pressed ? Qt.darker(Style.primaryColor, 1.2) :
                               resetViewButton.hovered ? Qt.lighter(Style.primaryColor, 1.2) :
                               Style.primaryColor
                        radius: 5
                    }

                    onPressed: {
                        modelRotationX = 0   // Reset to default front view
                        modelRotationY = 0
                        modelRotationZ = 0
                        scaleValue = 1.0
                        autoRotate.stop()
                    }
                }

                Button {
                    id: autoRotateButton
                    Layout.preferredWidth: 110
                    Layout.preferredHeight: 40

                    contentItem: Text {
                        text: autoRotate.running ? "Stop Rotate" : "Auto Rotate"
                        color: "white"
                        font.family: Style.buttonFont.family
                        font.pixelSize: Style.buttonFont.pixelSize
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: autoRotateButton.pressed ? Qt.darker(Style.primaryColor, 1.2) :
                               autoRotateButton.hovered ? Qt.lighter(Style.primaryColor, 1.2) :
                               Style.primaryColor
                        radius: 5
                    }

                    onPressed: autoRotate.running = !autoRotate.running
                }
            }

            Button {
                id: tryNowButton
                Layout.preferredWidth: 240
                Layout.preferredHeight: 50
                Layout.alignment: Qt.AlignHCenter

                contentItem: Text {
                    text: "Try Now"
                    color: "white"
                    font.family: Style.buttonFont.family
                    font.pixelSize: Style.buttonFont.pixelSize * 1.2
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: tryNowButton.pressed ? Qt.darker(Style.primaryColor, 1.2) :
                           tryNowButton.hovered ? Qt.lighter(Style.primaryColor, 1.2) :
                           Style.primaryColor
                    radius: 8
                }

                onPressed: {
                    qmlManager.tryOnGarment(garmentId)
                    stackView.push("CameraPage.qml")
                }
            }
        }
    }

    // Back Button
    Button {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 20
        width: 100
        height: 40

        contentItem: Text {
            text: "← Back"
            color: "white"
            font.family: Style.buttonFont.family
            font.pixelSize: Style.buttonFont.pixelSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: backButton.pressed ? Qt.darker(Style.primaryColor, 1.2) :
                   backButton.hovered ? Qt.lighter(Style.primaryColor, 1.2) :
                   Style.primaryColor
            radius: 5
        }

        onPressed: stackView.pop()
    }
}
