import QtQuick 2.12
import QtQuick.Controls 2.12

Item {
    id: root

    property string pageTitle:   "Page"
    property string accentColor: "#1a7fd4"
    signal back()

    // Dark background
    Rectangle {
        anchors.fill: parent
        color: "#080d18"
    }

    // Title bar
    Rectangle {
        id: titleBar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 60
        color:  "#0d1530"
        border.color: "#1a3060"; border.width: 1

        // Accent underline
        Rectangle {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
            height: 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0;  color: "transparent" }
                GradientStop { position: 0.2;  color: root.accentColor }
                GradientStop { position: 0.8;  color: root.accentColor }
                GradientStop { position: 1.0;  color: "transparent" }
            }
        }

        // Back button
        Rectangle {
            id: backBtn
            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
            anchors.leftMargin: 16
            width: 110; height: 36; radius: 6
            color:  backMa.containsMouse ? "#1a3060" : "#111827"
            border.color: "#2a5fa8"; border.width: 1
            Behavior on color { ColorAnimation { duration: 120 } }

            Text {
                anchors.centerIn: parent
                text: "◀  Back"
                font.pixelSize: 15
                color: "#94b8e8"
                renderType: Text.NativeRendering
            }

            MouseArea {
                id: backMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape:  Qt.PointingHandCursor
                onClicked:    root.back()
            }

            scale: backMa.containsPress ? 0.95 : 1.0
            Behavior on scale { NumberAnimation { duration: 80 } }
        }

        // Page title
        Text {
            anchors.centerIn: parent
            text:  root.pageTitle
            font.pixelSize: 22
            font.weight: Font.Light
            
            color: "#d4e4ff"
            renderType: Text.NativeRendering
        }
    }

    // Content area — children of BasePage go here
    default property alias content: contentArea.data
    Item {
        id: contentArea
        anchors {
            top: titleBar.bottom; left: parent.left
            right: parent.right;  bottom: parent.bottom
        }
    }
}
