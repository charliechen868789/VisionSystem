import QtQuick 2.12
import QtQuick.Layouts 1.12

// Persistent top overlay — visible on every page, including home.
Rectangle {
    id: statusBar
    height: 32
    color: "#0a0e1ae6"
    z: 100

    property string currentTime: ""
    property string currentDate: ""

    function updateClock() {
        var now = new Date()
        currentTime = Qt.formatTime(now, "hh:mm")
        currentDate = Qt.formatDate(now, "ddd d MMM")
    }
    Component.onCompleted: updateClock()
    Timer { interval: 1000; running: true; repeat: true; onTriggered: statusBar.updateClock() }

    RowLayout {
        anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
        spacing: 12

        Text {
            text: statusBar.currentDate
            color: "#8aaed4"; font.pixelSize: 13
            renderType: Text.NativeRendering
        }

        Item { Layout.fillWidth: true }

        // WiFi signal glyph — rising bars, filled count driven by real
        // signal strength (backend.wifiSignal, 0-100%, from SystemWorker
        // reading /proc/net/wireless). All gray when disconnected.
        Item {
            width: 26; height: 18
            Layout.alignment: Qt.AlignVCenter
            Repeater {
                model: 4
                Rectangle {
                    width: 4
                    height: 4 + index * 4
                    radius: 1
                    x: index * 7
                    y: 18 - height
                    color: (backend.wifiUp && backend.wifiSignal >= (index + 1) * 25)
                           ? "#3dbf6e" : "#3a4a60"
                }
            }
        }

        Text {
            text: statusBar.currentTime
            color: "#ffffff"; font.pixelSize: 15; font.weight: Font.DemiBold
            renderType: Text.NativeRendering
        }
    }
}
