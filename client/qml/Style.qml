// qml/Style.qml
pragma Singleton
import QtQuick

QtObject {
    id: style

    // Colors
    property color primaryColor: "#32857e"
    property color secondaryColor: "#3498db"
    property color backgroundColor: "#ecf0f1"

    // Button properties
    property int buttonWidth: 200
    property int buttonHeight: 60
    property int buttonRadius: 10

    property font buttonFont: Qt.font({
        family: "Arial",
        pointSize: 16,
        bold: true
    })

    // Spacing and sizes
    property int gridSpacing: 10
    property int garmentPreviewSize: 100
    property int gridDelegateSpacing: 10

    // Other colors
    property color unavailableColor: "#cc0000"
    property color imageOverlayColor: "#80000000"
}
