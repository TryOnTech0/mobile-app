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
            visible: true // Make this visible if 3D model fails
        }

        // 3D Viewport - comment out temporarily to debug with just the image
        Scene3D {
            id: scene3d
            anchors.fill: parent
            focus: true
            visible: true // Set to true when 3D model is working

            Entity {
                id: sceneRoot

                components: [
                    RenderSettings {
                        activeFrameGraph: ForwardRenderer {
                            camera: mainCamera
                            clearColor: "lightgray"
                        }
                    },
                    // ObjectPicker {
                    //     id: modelPicker
                    //     hoverEnabled: true
                    //     dragEnabled: true

                    //     onMoved: (pick) => {
                    //         // Calculate rotation based on mouse delta
                    //         var dx = pick.x - pick.previousX
                    //         var dy = pick.y - pick.previousY
                    //         garmentTransform.rotation = garmentTransform.rotation
                    //             .times(Quaternion.fromEulerAngles(dy * 0.5, dx * 0.5, 0))
                    //     }
                    // },
                    InputSettings {}
                ]

                // Main Camera
                Camera {
                    id: mainCamera
                    position: Qt.vector3d(0, 0, 3)
                    viewCenter: Qt.vector3d(0, 0, 0)
                }

                // Orbit Camera Controller (for mouse interaction)
                OrbitCameraController {
                    camera: mainCamera
                    linearSpeed: 50
                    lookSpeed: 180
                }

                // Light Source
                Entity {
                    components: [
                        DirectionalLight {
                            intensity: 0.8
                            worldDirection: Qt.vector3d(0, -1, 0)
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
                        // NumberAnimation {
                        //     id: autoRotate
                        //     target: garmentTransform
                        //     property: "rotationY"
                        //     from: 0
                        //     to: 360
                        //     duration: 10000
                        //     loops: Animation.Infinite
                        //     running: false // Start with mouse control only
                        // },
                        PhongMaterial {
                            id: garmentMaterial
                            diffuse: "white"
                            ambient: "gray"
                        },
                        Transform {
                                                   id: garmentTransform
                                                   rotation: fromEulerAngles(0, 180, 0)  // Adjust orientation if needed
                                               }
                        // Transform {
                        //     id: garmentTransform
                        //     property real rotationX: 0
                        //     property real rotationY: 0
                        //     property real rotationZ: 0

                        //     matrix: {
                        //         var m = Qt.matrix4x4()
                        //         m.rotate(rotationX, Qt.vector3d(1, 0, 0))
                        //         m.rotate(rotationY, Qt.vector3d(0, 1, 0))
                        //         m.rotate(rotationZ, Qt.vector3d(0, 0, 1))
                        //         return m
                        //     }
                        // }
                    ]
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

    // Control Buttons
    RowLayout {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 20
        spacing: 20

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

        // Button {
        //     text: "Rotate Left"
        //     onClicked: garmentTransform.rotationY -= 45
        // }

        // Button {
        //     text: "Rotate Right"
        //     onClicked: garmentTransform.rotationY += 45
        // }

        // Button {
        //     text: "Auto Rotate"
        //     onClicked: autoRotate.running = !autoRotate.running
        // }
    }


}
