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
    property string garmentId  // Pass this from GarmentSelectionPage
    property url previewImage  // Pass this from GarmentSelectionPage

    Component.onCompleted: {
        console.log("Style properties:",
            "Primary color:", Style.primaryColor,
            "Background color:", Style.backgroundColor,
            "Button font:", Style.buttonFont
        )
    }

    background: Rectangle { color: Style.backgroundColor }

    QMLManager {
        id: qmlManager
    }

    // Layout with preview image as fallback
    Item {
        id: contentArea
        anchors.fill: parent

        // Preview Image (fallback for 3D model)
        Image {
            id: previewImageView
            anchors.fill: parent
            source: previewImage
            fillMode: Image.PreserveAspectFit
            visible: !scene3d.visible // Only show if 3D model isn't visible
        }

        // 3D Viewport
        Scene3D {
            id: scene3d
            anchors.fill: parent
            focus: true
            visible: true // Set to true when 3D model is working
            aspects: ["input", "logic"]
            multisample: true

            Entity {
                id: sceneRoot

                components: [
                    RenderSettings {
                        activeFrameGraph: ForwardRenderer {
                            camera: mainCamera
                            clearColor: "lightgray"
                        }
                    },
                    InputSettings { }
                ]

                // Main Camera
                Camera {
                    id: mainCamera
                    position: Qt.vector3d(0, 0, 3)
                    viewCenter: Qt.vector3d(0, 0, 0)
                }

                // Light Sources
                Entity {
                    components: [
                        DirectionalLight {
                            intensity: 0.8
                            worldDirection: Qt.vector3d(0, -1, 0)
                        }
                    ]
                }

                // Ambient light for better visibility
                Entity {
                    components: [
                        DirectionalLight {
                            intensity: 0.4
                            worldDirection: Qt.vector3d(1, -0.5, 1)
                        }
                    ]
                }

                // Load .obj Model
                Entity {
                    id: modelEntity

                    components: [
                        Mesh {
                            id: garmentMesh
                            source: "qrc:/garments/" + garmentId.split('_')[0] + "/model.obj"  // Extracts "shirt" from "shirt_001"
                        },
                        PhongMaterial {
                            id: garmentMaterial
                            diffuse: "white"
                            ambient: "gray"
                            shininess: 50
                        },
                        Transform {
                            id: garmentTransform
                            // Start with appropriate orientation - rotate around X axis -90 degrees
                            rotationX: -90
                            // We'll apply additional rotation in the animation
                        },
                        // Custom object picker for drag rotation
                        ObjectPicker {
                            id: modelPicker
                            hoverEnabled: true
                            dragEnabled: true

                            onPressed: function(pick) {
                                // Store initial drag position
                                lastX = pick.x
                                lastY = pick.y
                                autoRotate.pause() // Pause auto-rotation when manually rotating
                            }

                            onReleased: function(pick) {
                                if (autoRotateActive) {
                                    autoRotate.resume() // Resume auto-rotation if it was active
                                }
                            }

                            onMoved: function(pick) {
                                if (!pick.valid)
                                    return

                                // Calculate rotation based on mouse delta
                                var dx = pick.x - lastX
                                var dy = pick.y - lastY

                                // Update rotation based on mouse movement
                                if (Math.abs(dx) > Math.abs(dy)) {
                                    // Horizontal movement - rotate around y-axis
                                    garmentTransform.rotationY += dx * 0.5
                                } else {
                                    // Vertical movement - rotate around x-axis (limited range to prevent flipping)
                                    var newRotationX = garmentTransform.rotationX + dy * 0.5
                                    // Limit vertical rotation to avoid gimbal lock issues
                                    if (newRotationX > -150 && newRotationX < -30) {
                                        garmentTransform.rotationX = newRotationX
                                    }
                                }

                                // Update last position
                                lastX = pick.x
                                lastY = pick.y
                            }

                            // Store last mouse position for calculating deltas
                            property real lastX: 0
                            property real lastY: 0
                        }
                    ]

                    // Auto-rotation animation
                    property bool autoRotateActive: false

                    // Auto-rotation timer-based approach
                    Timer {
                        id: autoRotate
                        interval: 16 // ~60fps
                        running: false
                        repeat: true
                        property real rotationSpeed: 1.0 // degrees per frame

                        function pause() {
                            running = false
                        }

                        function resume() {
                            running = modelEntity.autoRotateActive
                        }

                        onTriggered: {
                            // Rotate around Y axis for continuous spinning effect
                            garmentTransform.rotationY += rotationSpeed
                        }
                    }
                }
            }
        }

        // Debug information - displays the garment ID and model path
        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            color: "#80000000"
            width: debugText.width + 20
            height: debugText.height + 10
            visible: true  // Set to false in production

            Text {
                id: debugText
                anchors.centerIn: parent
                text: "ID: " + garmentId + "\nPath: qrc:/garments/" + garmentId.split('_')[0] + "/model.obj"
                color: "white"
                font.pixelSize: 12
            }
        }
    }

    // Control Buttons - Two rows layout
    Column {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 20
        spacing: 15

        // Top row - Rotation controls
        RowLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 20

            Button {
                text: "Rotate Left"
                background: Rectangle {
                    color: Style.primaryColor
                    radius: 5
                }
                contentItem: Text {
                    text: parent.text
                    font: Style.buttonFont
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    garmentTransform.rotationY -= 45
                }
            }

            Button {
                id: autoRotateButton
                text: "Auto Rotate"
                background: Rectangle {
                    color: modelEntity.autoRotateActive ? "#808080" : Style.primaryColor
                    radius: 5
                }
                contentItem: Text {
                    text: parent.text
                    font: Style.buttonFont
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    modelEntity.autoRotateActive = !modelEntity.autoRotateActive
                    autoRotate.running = modelEntity.autoRotateActive
                }
            }

            Button {
                text: "Rotate Right"
                background: Rectangle {
                    color: Style.primaryColor
                    radius: 5
                }
                contentItem: Text {
                    text: parent.text
                    font: Style.buttonFont
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    garmentTransform.rotationY += 45
                }
            }
        }

        // Bottom row - Navigation buttons
        RowLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 40 // Larger spacing between navigation buttons

            Button {
                text: "Go Back"
                background: Rectangle {
                    color: Style.primaryColor
                    radius: 5
                }
                contentItem: Text {
                    text: parent.text
                    font: Style.buttonFont
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: stackView.pop()
            }

            Button {
                text: "Try On"
                background: Rectangle {
                    color: Style.primaryColor
                    radius: 5
                }
                contentItem: Text {
                    text: parent.text
                    font: Style.buttonFont
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    console.log("Trying on garment: " + garmentId);
                    qmlManager.tryOnGarment(garmentId);
                    stackView.push("CameraPage.qml");
                }
            }
        }
    }
}
