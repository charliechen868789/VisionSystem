import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

BasePage {
    pageTitle:   "CONNECTION"
    accentColor: "#e8a020"

    ColumnLayout {
        anchors { fill: parent; margins: 40 }
        spacing: 20

        Text {
            text: "Network & Interfaces"
            font.pixelSize: 18; font.weight: Font.Light
             color: "#c89040"
            renderType: Text.NativeRendering
        }

        // Interface rows — driven by backend properties
        Repeater {
            model: [
                { name: "Ethernet (eth0)",  ipProp: backend.ethIp,  okProp: backend.ethUp   },
                { name: "Wi-Fi (wlan0)",    ipProp: backend.wifiIp, okProp: backend.wifiUp  },
                { name: "CMD  TCP :8084",   ipProp: "0.0.0.0",      okProp: backend.cmdPort },
                { name: "DATA TCP :8083",   ipProp: "0.0.0.0",      okProp: backend.dataPort}
            ]

            delegate: Rectangle {
                Layout.fillWidth: true; height: 64; radius: 8
                color: "#0d1530"
                border.color: modelData.okProp ? "#1e3050" : "#3a2020"; border.width: 1
                Behavior on border.color { ColorAnimation { duration: 200 } }

                // Left accent bar
                Rectangle {
                    anchors { left: parent.left; top: parent.top; bottom: parent.bottom; margins: 1 }
                    width: 4; radius: 2
                    color: modelData.okProp ? "#e8a020" : "#803020"
                    Behavior on color { ColorAnimation { duration: 200 } }
                }

                ColumnLayout {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                    anchors.leftMargin: 24; anchors.rightMargin: 20
                    spacing: 2

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: modelData.name; font.pixelSize: 16; color: "#c0d0e8"
                            renderType: Text.NativeRendering; Layout.fillWidth: true
                        }
                        Text {
                            text: modelData.okProp
                                  ? (index < 2 ? "● Connected" : "● Listening")
                                  : "○ Disconnected"
                            font.pixelSize: 13
                            color: modelData.okProp ? "#50e89a" : "#d04040"
                            renderType: Text.NativeRendering
                            Behavior on color { ColorAnimation { duration: 200 } }
                        }
                    }

                    Text {
                        text: modelData.ipProp
                        font.pixelSize: 13; color: "#5a7a9a"
                        font.family: "Monospace"
                        renderType: Text.NativeRendering
                    }
                }
            }
        }

        // Scan button — calls backend.scanNetwork()
        Rectangle {
            width: 170; height: 42; radius: 8
            color:  scanMa.containsMouse ? "#2a4060" : "#1a2a40"
            border.color: "#e8a020"; border.width: 1
            Behavior on color { ColorAnimation { duration: 120 } }

            Text {
                anchors.centerIn: parent
                text: "⟳  Scan Network"; font.pixelSize: 15; color: "#e8a020"
                renderType: Text.NativeRendering
            }

            MouseArea {
                id: scanMa; anchors.fill: parent
                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: backend.scanNetwork()
            }

            scale: scanMa.containsPress ? 0.95 : 1.0
            Behavior on scale { NumberAnimation { duration: 80 } }
        }

        Item { Layout.fillHeight: true }
    }
}
