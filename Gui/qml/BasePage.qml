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

    // Title bar (topMargin leaves room for the global StatusBar overlay)
    Rectangle {
        id: titleBar
        anchors { top: parent.top; topMargin: 32; left: parent.left; right: parent.right }
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

    // Content area — children of BasePage go here (bottomMargin leaves
    // room for the global TaskBar overlay)
    default property alias content: contentArea.data
    Item {
        id: contentArea
        anchors {
            top: titleBar.bottom; left: parent.left
            right: parent.right;  bottom: parent.bottom
            bottomMargin: 56
        }
    }
}
