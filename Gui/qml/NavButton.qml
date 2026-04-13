import QtQuick 2.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12

Item {
    id: root

    property string label:           "Button"
    property string iconSource:      ""
    property string fallbackIconText:"•"

    signal clicked()

    width:  420
    height: 240

    // Drop shadow
    layer.enabled: true
    layer.effect: DropShadow {
        horizontalOffset: 0
        verticalOffset:   6
        radius:   20
        samples:  17
        color:    "#80000000"
        spread:   0.05
    }

    // Glass tile body
    Rectangle {
        id: bg
        anchors.fill: parent
        radius: 12
        color: ma.containsPress ? "#1e2a45"
             : ma.containsMouse ? "#17213a"
             : "#111827"
        border.color: ma.containsMouse ? "#2a5fa8" : "#232d45"
        border.width: 1
        Behavior on color       { ColorAnimation { duration: 120 } }
        Behavior on border.color{ ColorAnimation { duration: 120 } }

        // Top-edge highlight
        Rectangle {
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 1; radius: parent.radius
            color: "#3a4f70"; opacity: 0.5
        }

        // Reflection sheen
        Rectangle {
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: parent.height * 0.35; radius: parent.radius
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#18ffffff" }
                GradientStop { position: 1.0; color: "#00ffffff" }
            }
        }

        // Hover border glow
        Rectangle {
            anchors.fill: parent; radius: parent.radius
            color: "transparent"
            border.color: "#1a6ec7"
            border.width: ma.containsMouse ? 1 : 0
            opacity: ma.containsMouse ? 0.7 : 0
            Behavior on opacity      { NumberAnimation { duration: 150 } }
            Behavior on border.width { NumberAnimation { duration: 150 } }
        }
    }

    // Icon
    Image {
        id: icon
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
        anchors.topMargin: 40
        width: 88; height: 88
        source: root.iconSource
        fillMode: Image.PreserveAspectFit
        visible: status === Image.Ready
        smooth: true
        scale: ma.containsMouse ? 1.08 : 1.0
        Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutBack } }
    }

    // Fallback glyph
    Text {
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
        anchors.topMargin: 32
        visible: icon.status !== Image.Ready
        text:  root.fallbackIconText
        font.pixelSize: 76
        color: "#c0c8d8"
        renderType: Text.NativeRendering
        scale: ma.containsMouse ? 1.08 : 1.0
        Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutBack } }
    }

    // Label
    Text {
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
        anchors.bottomMargin: 22
        text:  root.label
        font.pixelSize: 20
        color: ma.containsMouse ? "#d4e4ff" : "#c8d4e8"
        renderType: Text.NativeRendering
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    // Interaction
    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape:  Qt.PointingHandCursor
        onClicked:    root.clicked()
        onPressed:    pressAnim.restart()
    }

    scale: ma.containsPress ? 0.96 : 1.0
    Behavior on scale { NumberAnimation { duration: 80 } }

    SequentialAnimation {
        id: pressAnim
        PropertyAnimation { target: bg; property: "opacity"; to: 0.65; duration: 60  }
        PropertyAnimation { target: bg; property: "opacity"; to: 1.0;  duration: 120 }
    }
}
