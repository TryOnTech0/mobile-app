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
            visible: false // Set to true when 3D model is working

            Entity {
                id: sceneRoot

                components: [
                    RenderSettings {
                        activeFrameGraph: ForwardRenderer {
                            camera: mainCamera
                            clearColor: "lightgray"
                        }
                    },
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
                        PhongMaterial {
                            id: garmentMaterial
                            diffuse: "white"
                            ambient: "gray"
                        },
                        Transform {
                            id: garmentTransform
                            rotation: fromEulerAngles(-90, 0, 0)  // Adjust orientation if needed
                        }
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
    }

    Component.onCompleted: {
        console.log("GarmentPreviewPage loaded with garmentId: " + garmentId);
        console.log("Preview image: " + previewImage);
    }
}
