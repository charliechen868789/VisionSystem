import QtQuick 2.12

Item {
    id: toggleRoot
    width: parent.width
    height: 54
    property string label: ""
    property bool value: false
    signal toggled(bool newVal)

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: "#0d1530"
        border.color: "#1e3050"

        Text {
            anchors.verticalCenter: parent.verticalCenter
            leftPadding: 20
            text: toggleRoot.label
            color: "#c0d0e8"
            font.pixelSize: 16
            renderType: Text.NativeRendering
        }

        Rectangle {
            id: track
            width: 42; height: 22; radius: 11
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 20
            color: toggleRoot.value ? "#9b3de8" : "#1a2540"
            border.color: "#1e3050"

            Behavior on color { ColorAnimation { duration: 120 } }

            Rectangle {
                width: 18; height: 18; radius: 9
                y: 2
                x: toggleRoot.value ? 22 : 2
                color: "#c0d0e8"
                Behavior on x { NumberAnimation { duration: 120 } }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    toggleRoot.value = !toggleRoot.value
                    toggleRoot.toggled(toggleRoot.value)
                }
            }
        }
    }
}