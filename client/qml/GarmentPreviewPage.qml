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
    property url modelObject

    // State variables for garment manipulation (model only)
    property real scaleValue: 1.0
    property real modelRotationX: -90  // Initial X rotation for typical OBJ models
    property real modelRotationY: 0    // No initial Y rotation
    property real modelRotationZ: 0    // No initial Z rotation
    property vector2d lastTouchPos: Qt.vector2d(0, 0)
    property bool isDragging: false

    Component.onCompleted: {
        console.log("Style properties:",
            "Primary color:", Style.primaryColor,
            "Background color:", Style.backgroundColor,
            "Button font:", Style.buttonFont
        )
        console.log("Model object URL:", modelObject)
        console.log("URL toString:", modelObject.toString())

        // Check file format
        if (modelObject.toString().includes(".glb")) {
            console.log("Loading GLB format")
        } else if (modelObject.toString().includes(".obj")) {
            console.log("Loading OBJ format")
        }
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
                    // Fixed camera position that won't move
                    position: Qt.vector3d(0, 0, 5)
                    viewCenter: Qt.vector3d(0, 0, 0)
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

                // Additional lighting for better GLB visualization
                Entity {
                    components: DirectionalLight {
                        intensity: 0.5
                        worldDirection: Qt.vector3d(1, 1, -1)
                    }
                }

                Entity {
                    components: DirectionalLight {
                        intensity: 0.3
                        worldDirection: Qt.vector3d(-1, -1, 1)
                    }
                }

                Entity {
                    id: modelEntity
                    enabled: true  // Use enabled instead of visible for Qt3D entities

                    components: [
                        Transform {
                            id: modelTransform
                            scale3D: Qt.vector3d(scaleValue, scaleValue, scaleValue)
                            rotationX: modelRotationX
                            rotationY: modelRotationY
                            rotationZ: modelRotationZ
                        },
                        SceneLoader {
                            id: sceneLoader
                            source: modelObject

                            Component.onCompleted: {
                                console.log("SceneLoader initialized with source:", source)
                                console.log("File exists check - URL:", source)

                                // Add more detailed logging
                                sceneLoader.statusChanged.connect(function() {
                                    console.log("SceneLoader status changed:", sceneLoader.status)
                                    console.log("Source URL:", sceneLoader.source)

                                    switch(sceneLoader.status) {
                                        case SceneLoader.None:
                                            console.log("SceneLoader: None - No source set")
                                            break;
                                        case SceneLoader.Loading:
                                            console.log("SceneLoader: Loading... File being processed")
                                            loadingIndicator.visible = true
                                            modelError.visible = false
                                            break;
                                        case SceneLoader.Ready:
                                            console.log("SceneLoader: Ready - Model loaded successfully!")
                                            console.log("Entity count:", sceneLoader.entity ? "Entity created" : "No entity")
                                            loadingIndicator.visible = false
                                            modelError.visible = false
                                            fallbackMesh.enabled = false
                                            break;
                                        case SceneLoader.Error:
                                            console.log("SceneLoader: Error loading model")
                                            console.log("Error details - Source:", sceneLoader.source)
                                            loadingIndicator.visible = false
                                            modelError.visible = true
                                            // Try fallback approaches
                                            tryFallbackLoading()
                                            break;
                                    }
                                })
                            }

                            // Add this function to try different loading methods
                            function tryFallbackLoading() {
                                console.log("Trying fallback loading methods...")

                                // Try with Mesh component instead
                                fallbackMesh.enabled = true

                                // If GLB fails, suggest OBJ format
                                if (source.toString().includes(".glb")) {
                                    console.log("GLB loading failed. Consider converting to OBJ format.")
                                    modelError.errorText = "GLB format issue. Try OBJ format instead."
                                }
                            }
                        }
                    ]
                }

                // Enhanced fallback mesh with better material
                Entity {
                    id: fallbackMesh
                    enabled: false  // Use enabled instead of visible for Qt3D entities

                    components: [
                        Transform {
                            scale3D: Qt.vector3d(scaleValue, scaleValue, scaleValue)
                            rotationX: modelRotationX
                            rotationY: modelRotationY
                            rotationZ: modelRotationZ
                        },
                        Mesh {
                            id: modelMesh
                            source: modelObject

                            Component.onCompleted: {
                                console.log("Fallback Mesh initialized with source:", source)

                                modelMesh.statusChanged.connect(function() {
                                    console.log("Fallback Mesh status:", modelMesh.status)

                                    if (modelMesh.status === Mesh.Error) {
                                        console.log("Error: Both SceneLoader and Mesh failed")
                                        console.log("File might be corrupted or unsupported format")
                                        modelError.errorText = "Model file is corrupted or unsupported format"
                                        modelError.visible = true
                                        // Show placeholder geometry
                                        showPlaceholderGeometry()
                                    } else if (modelMesh.status === Mesh.Ready) {
                                        console.log("Fallback mesh loaded successfully!")
                                        modelError.visible = false
                                    }
                                })
                            }

                            function showPlaceholderGeometry() {
                                // Replace mesh with a placeholder cube
                                modelMesh.source = ""  // Clear failed source
                                // You could add a CuboidMesh or other primitive here as ultimate fallback
                            }
                        },
                        // Enhanced material for better visibility
                        PhongMaterial {
                            id: modelMaterial
                            ambient: Qt.rgba(0.3, 0.3, 0.4, 1.0)
                            diffuse: Qt.rgba(0.7, 0.7, 0.8, 1.0)
                            specular: Qt.rgba(0.9, 0.9, 1.0, 1.0)
                            shininess: 80
                            // Add normal map if available
                            // normalMap: Texture2D { source: "normal_map.png" }
                        }
                    ]
                }
            }
        }

        // Loading indicator
        Rectangle {
            id: loadingIndicator
            anchors.centerIn: parent
            width: 200
            height: 100
            color: Qt.rgba(0.2, 0.2, 0.2, 0.8)
            radius: 8
            visible: false

            Column {
                anchors.centerIn: parent
                spacing: 10

                Text {
                    text: "Loading 3D Model..."
                    color: "white"
                    font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Rectangle {
                    width: 100
                    height: 4
                    color: "gray"
                    radius: 2
                    anchors.horizontalCenter: parent.horizontalCenter

                    Rectangle {
                        id: progressBar
                        width: 0
                        height: parent.height
                        color: Style.primaryColor
                        radius: parent.radius

                        SequentialAnimation on width {
                            running: loadingIndicator.visible
                            loops: Animation.Infinite
                            NumberAnimation { to: 100; duration: 1000 }
                            NumberAnimation { to: 0; duration: 1000 }
                        }
                    }
                }
            }
        }

        // Enhanced error display
        Rectangle {
            id: modelError
            anchors.centerIn: parent
            width: 350
            height: 200
            color: Qt.rgba(0.5, 0.2, 0.2, 0.9)
            radius: 8
            visible: false

            property string errorText: "Failed to load 3D model"

            Column {
                anchors.centerIn: parent
                spacing: 15

                Text {
                    text: "3D Model Loading Error"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 18
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    text: modelError.errorText
                    color: "white"
                    font.pixelSize: 14
                    anchors.horizontalCenter: parent.horizontalCenter
                    wrapMode: Text.WordWrap
                    width: parent.parent.width - 40
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    text: "File: " + modelObject.toString().split('/').pop()
                    color: "lightgray"
                    font.pixelSize: 12
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 10

                    Button {
                        text: "Retry"
                        onPressed: {
                            modelError.visible = false
                            // Force reload
                            var tempSource = sceneLoader.source
                            sceneLoader.source = ""
                            Qt.callLater(function() {
                                sceneLoader.source = tempSource
                            })
                        }
                    }

                    Button {
                        text: "Use Fallback"
                        onPressed: {
                            modelError.visible = false
                            fallbackMesh.enabled = true
                            sceneLoader.enabled = false
                        }
                    }
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

        // Enhanced Debug View
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            color: "#80000000"
            width: debugText.width + 20
            height: debugText.height + 10
            visible: true // Enable for debugging GLB loading

            Text {
                id: debugText
                anchors.centerIn: parent
                text: `Scale: ${scaleValue.toFixed(2)}, Rotation: (${modelRotationX.toFixed(1)}, ${modelRotationY.toFixed(1)})
SceneLoader: ${sceneLoader.status}, Fallback: ${fallbackMesh.enabled}
Source: ${modelObject.toString().split('/').pop()}`
                color: "white"
                font.pixelSize: 10
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
                        modelRotationX = -90  // Reset to default initial position
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
                    stackView.push("ARCamera.qml", {"garmentId": garmentId})
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
