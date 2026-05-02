import QtQuick 2.12

Item {
    id: dropRoot
    width: parent.width
    height: open ? 54 + list.length * 42 : 54
    property string label: ""
    property var list: []
    property int selected: 0
    property bool open: false
    signal selected_changed(int idx)

    Behavior on height { NumberAnimation { duration: 150 } }

    Rectangle {
        id: dropHeader
        width: parent.width
        height: 54
        radius: 8
        color: dropRoot.open ? "#12253d" : "#0d1530"
        border.color: dropRoot.open ? "#9b3de8" : "#1e3050"

        Behavior on border.color { ColorAnimation { duration: 120 } }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            leftPadding: 20
            text: dropRoot.label + ": " + dropRoot.list[dropRoot.selected]
            color: "#c0d0e8"
            font.pixelSize: 16
            renderType: Text.NativeRendering
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 20
            text: dropRoot.open ? "▲" : "▼"
            color: "#9b3de8"
            font.pixelSize: 12
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: dropRoot.open = !dropRoot.open
        }
    }

    Column {
        y: 58
        width: parent.width
        spacing: 2
        visible: dropRoot.open
        clip: true

        Repeater {
            model: dropRoot.list
            Rectangle {
                width: dropRoot.width
                height: 40
                radius: 6
                color: dropItemMouse.containsMouse ? "#12253d" : "#0a1128"
                border.color: dropRoot.selected === index ? "#9b3de8" : "#1e3050"

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    leftPadding: 20
                    text: modelData
                    color: dropRoot.selected === index ? "#9b3de8" : "#c0d0e8"
                    font.pixelSize: 15
                    renderType: Text.NativeRendering
                }

                Text {
                    visible: dropRoot.selected === index
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 16
                    text: "✓"
                    color: "#9b3de8"
                    font.pixelSize: 14
                }

                MouseArea {
                    id: dropItemMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        dropRoot.selected = index
                        dropRoot.open = false
                        dropRoot.selected_changed(index)
                    }
                }
            }
        }
    }
}