import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Aeroboard 1.0

BasePage {
    id: videoPage
    pageTitle: "LIVE VIDEO"
    accentColor: "#9b3de8"
    onBack: { stackView.pop() }
    // Register VideoItem with backend when page is ready
    Component.onCompleted: {
        backend.registerVideoItem(videoSurface)   // pass the object ref
    }
    ColumnLayout {
        anchors { fill: parent; margins: 20 }
        spacing: 12

        // ── Video surface ─────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0a0f1e"
            radius: 8
            border.color: videoSurface.active ? "#9b3de8" : "#1e3050"
            border.width: videoSurface.active ? 2 : 1

            Behavior on border.color { ColorAnimation { duration: 300 } }

            VideoItem {
                id: videoSurface
                anchors { fill: parent; margins: 2 }
            }

            // No-signal overlay
            Column {
                anchors.centerIn: parent
                spacing: 10
                visible: !videoSurface.active

                Rectangle {
                    width: 48; height: 48; radius: 24
                    color: "#1e3050"
                    anchors.horizontalCenter: parent.horizontalCenter
                    Text {
                        anchors.centerIn: parent
                        text: "⏸"
                        font.pixelSize: 22
                        color: "#4a6080"
                    }
                }
                Text {
                    text: "Waiting for video stream..."
                    color: "#4a6080"
                    font.pixelSize: 13
                    anchors.horizontalCenter: parent.horizontalCenter
                    renderType: Text.NativeRendering
                }
            }
        }

        // ── Status bar ────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 56
            radius: 8
            color: "#0d1530"
            border.color: "#1e3050"

            RowLayout {
                anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
                spacing: 16

                // Stream status
                Row {
                    spacing: 6
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        anchors.verticalCenter: parent.verticalCenter
                        color: videoSurface.active ? "#44ff88" : "#ff4444"

                        SequentialAnimation on opacity {
                            running: videoSurface.active
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.3; duration: 800 }
                            NumberAnimation { to: 1.0; duration: 800 }
                        }
                    }
                    Text {
                        text: videoSurface.active ? "LIVE" : "NO SIGNAL"
                        color: videoSurface.active ? "#44ff88" : "#ff4444"
                        font.pixelSize: 12
                        font.letterSpacing: 1.5
                        font.weight: Font.Medium
                        anchors.verticalCenter: parent.verticalCenter
                        renderType: Text.NativeRendering
                    }
                }

                // Divider
                Rectangle { width: 1; height: 24; color: "#1e3050" }

                // AI label
                Row {
                    spacing: 6
                    Text {
                        text: "AI:"
                        color: "#6a7fa8"
                        font.pixelSize: 12
                        anchors.verticalCenter: parent.verticalCenter
                        renderType: Text.NativeRendering
                    }
                    Text {
                        text: backend.aiLabel !== "" ? backend.aiLabel : "—"
                        color: "#ffffff"
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        anchors.verticalCenter: parent.verticalCenter
                        renderType: Text.NativeRendering
                    }
                }

                // Divider
                Rectangle { width: 1; height: 24; color: "#1e3050" }

                // Confidence
                Row {
                    spacing: 6
                    Text {
                        text: "Conf:"
                        color: "#6a7fa8"
                        font.pixelSize: 12
                        anchors.verticalCenter: parent.verticalCenter
                        renderType: Text.NativeRendering
                    }
                    Text {
                        text: backend.aiConfidence > 0
                              ? (backend.aiConfidence * 100).toFixed(1) + "%"
                              : "—"
                        color: backend.aiConfidence > 0.8
                               ? "#44ff88"
                               : backend.aiConfidence > 0.5
                                 ? "#ffaa44" : "#ff6644"
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        anchors.verticalCenter: parent.verticalCenter
                        renderType: Text.NativeRendering
                    }
                }

                Item { Layout.fillWidth: true }

                // Frame seq
                Text {
                    text: "frame #" + backend.aiFrameSeq
                    color: "#334466"
                    font.pixelSize: 11
                    anchors.verticalCenter: parent.verticalCenter
                    renderType: Text.NativeRendering
                }
            }
        }
    }
}
