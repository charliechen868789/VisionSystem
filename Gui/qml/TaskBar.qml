import QtQuick 2.12
import QtQuick.Layouts 1.12

// Persistent bottom overlay — visible on every page, including home.
Rectangle {
    id: taskBar
    height: 56
    color: "#0a0e1ae6"
    z: 100

    property bool canGoBack: false
    signal backRequested()
    signal homeRequested()

    RowLayout {
        anchors { fill: parent; leftMargin: 20; rightMargin: 20 }
        spacing: 16

        Rectangle {
            width: 110; height: 40; radius: 8
            color: backMa.containsPress ? "#1a3060" : (backMa.containsMouse ? "#152540" : "#111827")
            border.color: taskBar.canGoBack ? "#2a5fa8" : "#1a2438"; border.width: 1
            opacity: taskBar.canGoBack ? 1.0 : 0.4
            Behavior on color { ColorAnimation { duration: 120 } }

            Text {
                anchors.centerIn: parent
                text: "◀  Back"; font.pixelSize: 15; color: "#94b8e8"
                renderType: Text.NativeRendering
            }
            MouseArea {
                id: backMa
                anchors.fill: parent
                hoverEnabled: true
                enabled: taskBar.canGoBack
                cursorShape: Qt.PointingHandCursor
                onClicked: taskBar.backRequested()
            }
            scale: backMa.containsPress ? 0.95 : 1.0
            Behavior on scale { NumberAnimation { duration: 80 } }
        }

        Item { Layout.fillWidth: true }

        Rectangle {
            width: 110; height: 40; radius: 8
            color: homeMa.containsPress ? "#1a3060" : (homeMa.containsMouse ? "#152540" : "#111827")
            border.color: "#2a5fa8"; border.width: 1
            Behavior on color { ColorAnimation { duration: 120 } }

            Text {
                anchors.centerIn: parent
                text: "⌂  Home"; font.pixelSize: 15; color: "#94b8e8"
                renderType: Text.NativeRendering
            }
            MouseArea {
                id: homeMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: taskBar.homeRequested()
            }
            scale: homeMa.containsPress ? 0.95 : 1.0
            Behavior on scale { NumberAnimation { duration: 80 } }
        }
    }
}
