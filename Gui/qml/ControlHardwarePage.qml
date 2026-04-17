import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

BasePage {
    pageTitle: "CONTROL HARDWARE"
    accentColor: "#1a7fd4"

    ColumnLayout {
        anchors { fill: parent; margins: 40 }
        spacing: 20

        Text {
            text: "Hardware Controls"
            font.pixelSize: 18
            font.weight: Font.Light
            color: "#8aaed4"
            renderType: Text.NativeRendering
        }

        Repeater {
            model: [
                { label: "GPIO Output 0", action: function(){ backend.gpio0 = !backend.gpio0 } },
                { label: "GPIO Output 1", action: function(){ backend.gpio1 = !backend.gpio1 } },
                { label: "PWM Enable",    action: function(){ backend.pwmEnable = !backend.pwmEnable } },
                { label: "SPI Bus",       action: function(){ backend.spiBus = !backend.spiBus } },
                { label: "About", type: "page", page: "AboutPage.qml" }
            ]

            delegate: Rectangle {
                Layout.fillWidth: true
                height: 54
                radius: 8

                property bool hovered: false

                color: hovered ? "#111c36" : "#0d1530"
                border.color: hovered ? "#2a9fe4" : "#1e3050"
                border.width: 1

                Behavior on color { ColorAnimation { duration: 120 } }

                Text {
                    anchors.centerIn: parent
                    text: modelData.label
                    font.pixelSize: 16
                    color: hovered ? "#ffffff" : "#c0d0e8"
                    renderType: Text.NativeRendering
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor

                    onEntered: hovered = true
                    onExited: hovered = false

                    onClicked: {
                        // 1. Check if it's a page navigation
                        if (modelData.type === "page") {
                            stackView.push(modelData.page)
                        }
                        // 2. Otherwise, check if an action exists and run it
                        else if (modelData.action !== undefined) {
                            modelData.action()
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
