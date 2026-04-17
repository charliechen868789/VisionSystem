import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

BasePage {
    id: aboutPage
    pageTitle: "SYSTEM INFO"
    accentColor: "#1a7fd4"
    onBack: { stackView.pop() }

    Component.onCompleted: backend.requestSystemInfo()

    ColumnLayout {
        anchors { fill: parent; margins: 40 }
        spacing: 20

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 420
            color: "#0d1530"
            radius: 8
            border.color: "#1e3050"

            ColumnLayout {
                anchors { fill: parent; margins: 20 }
                spacing: 15

                Text {
                    text: "Hardware Details"
                    color: "#8aaed4"; font.pixelSize: 18
                    renderType: Text.NativeRendering
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Device:";      color: "#c0d0e8"; font.pixelSize: 16; Layout.fillWidth: true }
                    Text { text: "Jetson Nano";  color: "#ffffff"; font.pixelSize: 16; font.weight: Font.DemiBold }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Kernel:";         color: "#c0d0e8"; font.pixelSize: 16; Layout.fillWidth: true }
                    Text { text: "4.9.253-tegra";   color: "#ffffff"; font.pixelSize: 16; font.weight: Font.DemiBold }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Status:";       color: "#c0d0e8"; font.pixelSize: 16; Layout.fillWidth: true }
                    Text { text: "Operational";   color: "#ffffff"; font.pixelSize: 16; font.weight: Font.DemiBold }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Temperature:";  color: "#c0d0e8"; font.pixelSize: 16; Layout.fillWidth: true }
                    Text {
                        text: backend.temperature.toFixed(1) + " °C"
                        color: backend.temperature > 70 ? "#ff4444" : "#ffffff"
                        font.pixelSize: 16; font.weight: Font.DemiBold
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "CPU:";  color: "#c0d0e8"; font.pixelSize: 16; Layout.fillWidth: true }
                    Text {
                        text: backend.cpuPercent.toFixed(1) + " %"
                        color: backend.cpuPercent > 85 ? "#ff4444" : "#ffffff"
                        font.pixelSize: 16; font.weight: Font.DemiBold
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Memory:"; color: "#c0d0e8"; font.pixelSize: 16; Layout.fillWidth: true }
                    Text {
                        text: backend.memPercent.toFixed(1) + " %"
                        color: backend.memPercent > 90 ? "#ff4444" : "#ffffff"
                        font.pixelSize: 16; font.weight: Font.DemiBold
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Uptime:"; color: "#c0d0e8"; font.pixelSize: 16; Layout.fillWidth: true }
                    Text { text: backend.uptime; color: "#ffffff"; font.pixelSize: 16; font.weight: Font.DemiBold }
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