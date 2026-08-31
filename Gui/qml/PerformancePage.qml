import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

BasePage {
    pageTitle:   "PERFORMANCE"
    accentColor: "#c04fd8"

    Component.onCompleted: backend.requestSystemInfo()

    function barColor(pct) {
        if (pct > 85) return "#ff4444"
        if (pct > 60) return "#e8a020"
        return "#c04fd8"
    }

    ColumnLayout {
        anchors { fill: parent; margins: 40 }
        spacing: 20

        Text {
            text: "Live Usage"
            font.pixelSize: 26; font.weight: Font.Light
            color: "#c890e0"
            renderType: Text.NativeRendering
        }

        Repeater {
            model: [
                { label: "CPU",        value: backend.cpuPercent, unit: "%",  max: 100 },
                { label: "GPU",        value: backend.gpuPercent, unit: "%",  max: 100 },
                { label: "Memory",     value: backend.memPercent, unit: "%",  max: 100 },
                { label: "Temperature",value: backend.temperature,unit: "°C", max: 90  }
            ]

            delegate: Rectangle {
                Layout.fillWidth: true; height: 84; radius: 8
                color: "#0d1530"; border.color: "#1e3050"; border.width: 1

                ColumnLayout {
                    anchors { fill: parent; leftMargin: 24; rightMargin: 24; topMargin: 14; bottomMargin: 14 }
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: modelData.label; font.pixelSize: 22; color: "#c0d0e8"
                            renderType: Text.NativeRendering; Layout.fillWidth: true
                        }
                        Text {
                            text: modelData.value.toFixed(1) + " " + modelData.unit
                            font.pixelSize: 22; font.weight: Font.DemiBold
                            color: barColor(100 * modelData.value / modelData.max)
                            renderType: Text.NativeRendering
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true; height: 10; radius: 5
                        color: "#1a2438"

                        Rectangle {
                            width: parent.width * Math.min(1.0, modelData.value / modelData.max)
                            height: parent.height; radius: 5
                            color: barColor(100 * modelData.value / modelData.max)
                            Behavior on width { NumberAnimation { duration: 250; easing.type: Easing.OutQuad } }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true; height: 68; radius: 8
            color: "#0d1530"; border.color: "#1e3050"; border.width: 1
            RowLayout {
                anchors { fill: parent; leftMargin: 24; rightMargin: 24 }
                Text {
                    text: "Uptime"; font.pixelSize: 22; color: "#c0d0e8"
                    renderType: Text.NativeRendering; Layout.fillWidth: true
                }
                Text {
                    text: backend.uptime; font.pixelSize: 22; color: "#ffffff"; font.weight: Font.DemiBold
                    renderType: Text.NativeRendering
                }
            }
        }

        Item { Layout.fillHeight: true }
    }

    // Poll every 2 seconds
    Timer {
        interval: 2000
        running: true
        repeat: true
        onTriggered: backend.requestSystemInfo()
    }
}
