// qml/Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ARClothTryOn 1.0

ApplicationWindow {
    id: window
    visible: true
    width: 360
    height: 720
    title: qsTr("AR Cloth Try On")

    // Enable console.log output in QML
    Component.onCompleted: {
        console.log("Main.qml loaded")
    }

    // Main stack view
    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: "qrc:/ARClothTryOn/qml/MainPage.qml"

        onCurrentItemChanged: {
            console.log("Current item changed: " + currentItem)
        }
    }
}
