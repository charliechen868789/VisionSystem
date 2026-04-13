import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

BasePage {
    pageTitle:   "VIDEO CONTROL"
    accentColor: "#9b3de8"

    ColumnLayout {
        anchors { fill: parent; margins: 40 }
        spacing: 20

        Text {
            text: "Video Settings"
            font.pixelSize: 18; font.weight: Font.Light
             color: "#a888d0"
            renderType: Text.NativeRendering
        }

        // Resolution
        Rectangle {
            Layout.fillWidth: true; height: 54; radius: 8
            color: "#0d1530"; border.color: "#1e3050"; border.width: 1

            RowLayout {
                anchors { fill: parent; leftMargin: 20; rightMargin: 20 }
                Text {
                    text: "Resolution"; font.pixelSize: 16; color: "#c0d0e8"
                    renderType: Text.NativeRendering; Layout.fillWidth: true
                }
                ComboBox {
                    model: ["1920×1080", "1280×720", "640×480"]
                    currentIndex: backend.resolution
                    onActivated: backend.resolution = index
                    font.pixelSize: 14
                    width: 160
                    background: Rectangle {
                        color: "#1a2540"; radius: 6
                        border.color: "#2a5fa8"; border.width: 1
                    }
                    contentItem: Text {
                        leftPadding: 10; text: parent.displayText
                        font: parent.font; color: "#c0d0e8"
                        verticalAlignment: Text.AlignVCenter
                        renderType: Text.NativeRendering
                    }
                }
            }
        }

        // Brightness
        Rectangle {
            Layout.fillWidth: true; height: 54; radius: 8
            color: "#0d1530"; border.color: "#1e3050"; border.width: 1

            RowLayout {
                anchors { fill: parent; leftMargin: 20; rightMargin: 20 }
                Text {
                    text: "Brightness"; font.pixelSize: 16; color: "#c0d0e8"
                    renderType: Text.NativeRendering; Layout.preferredWidth: 120
                }
                Slider {
                    id: brightSlider
                    from: 0; to: 100; value: backend.brightness
                    Layout.fillWidth: true
                    onValueChanged: backend.brightness = Math.round(value)

                    background: Rectangle {
                        x: brightSlider.leftPadding
                        y: brightSlider.topPadding + brightSlider.availableHeight / 2 - height / 2
                        width: brightSlider.availableWidth; height: 4; radius: 2
                        color: "#1e3050"
                        Rectangle {
                            width: brightSlider.visualPosition * parent.width
                            height: parent.height; radius: 2; color: "#9b3de8"
                        }
                    }
                    handle: Rectangle {
                        x: brightSlider.leftPadding + brightSlider.visualPosition * (brightSlider.availableWidth - width)
                        y: brightSlider.topPadding + brightSlider.availableHeight / 2 - height / 2
                        width: 20; height: 20; radius: 10
                        color: "#c090ff"; border.color: "#9b3de8"; border.width: 2
                    }
                }
                Text {
                    text: Math.round(brightSlider.value) + "%"
                    font.pixelSize: 14; color: "#9b3de8"
                    Layout.preferredWidth: 44
                    renderType: Text.NativeRendering
                }
            }
        }

        // Video source selector
        Repeater {
            model: ["MIPI CSI-2", "USB Camera", "File Playback"]
            delegate: Rectangle {
                Layout.fillWidth: true; height: 54; radius: 8
                color: srcMa.containsMouse ? "#12253d" : "#0d1530"
                border.color: backend.videoSource === index ? "#9b3de8" : "#1e3050"
                border.width: 1
                Behavior on color { ColorAnimation { duration: 100 } }
                Behavior on border.color { ColorAnimation { duration: 100 } }

                Text {
                    anchors { left: parent.left; verticalCenter: parent.verticalCenter; leftMargin: 20 }
                    text: modelData; font.pixelSize: 16; color: "#c0d0e8"
                    renderType: Text.NativeRendering
                }
                Text {
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 20 }
                    text: backend.videoSource === index ? "● Active" : "○ Inactive"
                    font.pixelSize: 14
                    color: backend.videoSource === index ? "#50e89a" : "#4a5a70"
                    renderType: Text.NativeRendering
                }
                MouseArea {
                    id: srcMa; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: backend.videoSource = index
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
