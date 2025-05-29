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
    QMLManager {
        id: qmlManager
    }

    

    // Main stack view
    StackView {
        id: stackView
        anchors.fill: parent
        // initialItem: "qrc:/ARClothTryOn/qml/MainPage.qml"
        //initialItem: "qrc:/ARClothTryOn/qml/AuthorizationPage.qml"
        initialItem: AuthorizationPage {}
        onCurrentItemChanged: {
            console.log("Current item changed: " + currentItem)
        }
    }
}
