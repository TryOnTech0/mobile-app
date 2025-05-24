// qml/Style.qml
pragma Singleton
import QtQuick

QtObject {
    id: style

    property font buttonFont: Qt.font({
        family: "Arial",
        pixelSize: 16,
        weight: Font.Bold
    })

    property font inputFont: Qt.font({
        family: "Arial",
        pixelSize: 14
    })

    // Text field background component (remove 'Component' wrapper)
    property Rectangle textFieldBackground: Rectangle {
        color: "white"
        border.color: "#cccccc"
        radius: 4
        implicitWidth: 200
        implicitHeight: 40
    }

    // Colors
    property color primaryColor: "#32857e"
    property color secondaryColor: "#3498db"
    property color backgroundColor: "#ecf0f1"


    // Spacing and sizes
    // property int gridSpacing: 10
    property int garmentPreviewSize: 100
    property int gridDelegateSpacing: 10

    // Other colors
    property color unavailableColor: "#cc0000"
    property color imageOverlayColor: "#80000000"

    property color primaryTextColor: "#32857e"
    property color secondaryTextColor: "#666666"
    property color errorColor: "#d32f2f"

    property int buttonHeight: 48
    property int buttonWidth: 200
    property int buttonRadius: 4
    property int gridSpacing: 8


}
