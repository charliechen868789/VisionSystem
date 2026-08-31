import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

BasePage {
    pageTitle:   "SETTINGS"
    accentColor: "#3dbf6e"

    signal systemSettingsRequested()
    signal informationRequested()
    signal networkInfoRequested()
    signal performanceRequested()

    ColumnLayout {
        anchors { fill: parent; margins: 40 }
        spacing: 20

        Rectangle {
            Layout.fillWidth: true; height: 68; radius: 8
            color: sysMa.containsMouse ? "#123018" : "#0d1530"
            border.color: "#1e3050"; border.width: 1
            Behavior on color { ColorAnimation { duration: 120 } }

            RowLayout {
                anchors { fill: parent; leftMargin: 24; rightMargin: 24 }
                Text {
                    text: "System Settings"; font.pixelSize: 22; color: "#c0d0e8"
                    renderType: Text.NativeRendering; Layout.fillWidth: true
                }
                Text {
                    text: "▶"; font.pixelSize: 20; color: "#3dbf6e"
                    renderType: Text.NativeRendering
                }
            }
            MouseArea {
                id: sysMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: systemSettingsRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true; height: 68; radius: 8
            color: infoMa.containsMouse ? "#123018" : "#0d1530"
            border.color: "#1e3050"; border.width: 1
            Behavior on color { ColorAnimation { duration: 120 } }

            RowLayout {
                anchors { fill: parent; leftMargin: 24; rightMargin: 24 }
                Text {
                    text: "Information"; font.pixelSize: 22; color: "#c0d0e8"
                    renderType: Text.NativeRendering; Layout.fillWidth: true
                }
                Text {
                    text: "▶"; font.pixelSize: 20; color: "#3dbf6e"
                    renderType: Text.NativeRendering
                }
            }
            MouseArea {
                id: infoMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: informationRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true; height: 68; radius: 8
            color: netMa.containsMouse ? "#123018" : "#0d1530"
            border.color: "#1e3050"; border.width: 1
            Behavior on color { ColorAnimation { duration: 120 } }

            RowLayout {
                anchors { fill: parent; leftMargin: 24; rightMargin: 24 }
                Text {
                    text: "Network Info"; font.pixelSize: 22; color: "#c0d0e8"
                    renderType: Text.NativeRendering; Layout.fillWidth: true
                }
                Text {
                    text: "▶"; font.pixelSize: 20; color: "#3dbf6e"
                    renderType: Text.NativeRendering
                }
            }
            MouseArea {
                id: netMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: networkInfoRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true; height: 68; radius: 8
            color: perfMa.containsMouse ? "#123018" : "#0d1530"
            border.color: "#1e3050"; border.width: 1
            Behavior on color { ColorAnimation { duration: 120 } }

            RowLayout {
                anchors { fill: parent; leftMargin: 24; rightMargin: 24 }
                Text {
                    text: "Performance"; font.pixelSize: 22; color: "#c0d0e8"
                    renderType: Text.NativeRendering; Layout.fillWidth: true
                }
                Text {
                    text: "▶"; font.pixelSize: 20; color: "#3dbf6e"
                    renderType: Text.NativeRendering
                }
            }
            MouseArea {
                id: perfMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: performanceRequested()
            }
        }

        Item { Layout.fillHeight: true }
    }
}
