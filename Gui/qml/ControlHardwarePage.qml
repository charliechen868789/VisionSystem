import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

BasePage {
    pageTitle:   "CONTROL HARDWARE"
    accentColor: "#1a7fd4"

    ColumnLayout {
        anchors { fill: parent; margins: 40 }
        spacing: 20

        Text {
            text: "Hardware Controls"
            font.pixelSize: 18; font.weight: Font.Light
             color: "#8aaed4"
            renderType: Text.NativeRendering
        }

        // Each row: label + toggle bound to backend property
        Repeater {
            model: [
                { label: "GPIO Output 0", read: function(){ return backend.gpio0    }, write: function(v){ backend.gpio0     = v } },
                { label: "GPIO Output 1", read: function(){ return backend.gpio1    }, write: function(v){ backend.gpio1     = v } },
                { label: "PWM Enable",    read: function(){ return backend.pwmEnable}, write: function(v){ backend.pwmEnable = v } },
                { label: "SPI Bus",       read: function(){ return backend.spiBus   }, write: function(v){ backend.spiBus    = v } }
            ]

            delegate: Rectangle {
                Layout.fillWidth: true
                height: 54; radius: 8
                color: "#0d1530"
                border.color: "#1e3050"; border.width: 1

                RowLayout {
                    anchors { fill: parent; leftMargin: 20; rightMargin: 20 }

                    Text {
                        text: modelData.label
                        font.pixelSize: 16; color: "#c0d0e8"
                        renderType: Text.NativeRendering
                        Layout.fillWidth: true
                    }

                    // Toggle pill
                    Rectangle {
                        id: tog
                        width: 54; height: 28; radius: 14
                        property bool on: modelData.read()
                        color:        on ? "#1a7fd4" : "#1e2a40"
                        border.color: on ? "#2a9fe4" : "#2a3a54"
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 130 } }

                        Rectangle {
                            x: parent.on ? parent.width - width - 3 : 3
                            anchors.verticalCenter: parent.verticalCenter
                            width: 22; height: 22; radius: 11
                            color: parent.on ? "#ffffff" : "#4a5a70"
                            Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape:  Qt.PointingHandCursor
                            onClicked: {
                                tog.on = !tog.on
                                modelData.write(tog.on)
                            }
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
