import QtQuick 2.12

// Invisible hotspot laid over a tile already drawn in the background image.
// Shows a very subtle highlight on press so the user gets tactile feedback
// without obscuring the artwork underneath.

Item {
    id: root
    signal tapped()

    // Subtle press overlay — fully transparent at rest
    Rectangle {
        anchors.fill: parent
        radius: 12
        color: ma.containsPress ? "#30ffffff" : "transparent"
        border.color: ma.containsPress ? "#60ffffff" : "transparent"
        border.width: 1
        Behavior on color { ColorAnimation { duration: 80 } }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.tapped()
    }

    // Scale-down on press for tactile feel
    scale: ma.containsPress ? 0.97 : 1.0
    Behavior on scale { NumberAnimation { duration: 80 } }
}
