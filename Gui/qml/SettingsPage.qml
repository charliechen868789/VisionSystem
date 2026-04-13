import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

BasePage {
    pageTitle:   "SETTINGS"
    accentColor: "#3dbf6e"

    ColumnLayout {
        anchors { fill: parent; margins: 40 }
        spacing: 20

        Text {
            text: "System Settings"
            font.pixelSize: 18; font.weight: Font.Light
             color: "#70c890"
            renderType: Text.NativeRendering
        }

        Repeater {
            model: [
                { label: "Auto-start on Boot", get: function(){ return backend.autoStart    }, set: function(v){ backend.autoStart    = v } },
                { label: "Debug Logging",       get: function(){ return backend.debugLogging }, set: function(v){ backend.debugLogging = v } },
                { label: "Watchdog Timer",      get: function(){ return backend.watchdog     }, set: function(v){ backend.watchdog     = v } },
                { label: "Low-power Mode",      get: function(){ return backend.lowPower     }, set: function(v){ backend.lowPower     = v } }
            ]

            delegate: Rectangle {
                Layout.fillWidth: true; height: 54; radius: 8
                color: "#0d1530"; border.color: "#1e3050"; border.width: 1

                RowLayout {
                    anchors { fill: parent; leftMargin: 20; rightMargin: 20 }
                    Text {
                        text: modelData.label; font.pixelSize: 16; color: "#c0d0e8"
                        renderType: Text.NativeRendering; Layout.fillWidth: true
                    }
                    Rectangle {
                        id: tog
                        width: 54; height: 28; radius: 14
                        property bool on: modelData.get()
                        color:        on ? "#3dbf6e" : "#1e2a40"
                        border.color: on ? "#60df8e" : "#2a3a54"; border.width: 1
                        Behavior on color { ColorAnimation { duration: 130 } }
                        Rectangle {
                            x: parent.on ? parent.width - width - 3 : 3
                            anchors.verticalCenter: parent.verticalCenter
                            width: 22; height: 22; radius: 11
                            color: parent.on ? "#ffffff" : "#4a5a70"
                            Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: { tog.on = !tog.on; modelData.set(tog.on) }
                        }
                    }
                }
            }
        }

        // Firmware version (read-only)
        Rectangle {
            Layout.fillWidth: true; height: 54; radius: 8
            color: "#0d1530"; border.color: "#1e3050"; border.width: 1
            RowLayout {
                anchors { fill: parent; leftMargin: 20; rightMargin: 20 }
                Text {
                    text: "Firmware Version"; font.pixelSize: 16; color: "#c0d0e8"
                    renderType: Text.NativeRendering; Layout.fillWidth: true
                }
                Text {
                    text: backend.firmwareVersion; font.pixelSize: 14; color: "#3dbf6e"
                    renderType: Text.NativeRendering
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
