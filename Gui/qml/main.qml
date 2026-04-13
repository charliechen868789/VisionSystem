import QtQuick 2.12
import QtQuick.Controls 2.12

ApplicationWindow {
    id: root
    visible: true
    width:  1280
    height: 720
    title:  "System Launcher"
    color:  "#000000"

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: homePage
    }

    // ── HOME ──────────────────────────────────────────────────────────────
    Component {
        id: homePage

        Item {
            // Background image (your ChatGPT-generated artwork)
            Image {
                id: bgImage
                anchors.fill: parent
                source: "qrc:/background.png"
                fillMode: Image.PreserveAspectCrop
                visible: status === Image.Ready
                smooth: true
            }

            // Fallback gradient when image is absent
            Rectangle {
                anchors.fill: parent
                visible: bgImage.status !== Image.Ready
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#0a0e1a" }
                    GradientStop { position: 0.5; color: "#0d1530" }
                    GradientStop { position: 1.0; color: "#070b14" }
                }
            }

            // ── Invisible hotspots over the 4 tiles drawn in background.png ──
            // The image is 1280×720. The 4 tiles are arranged in a 2×2 grid
            // centred on the image. Adjust x/y/width/height if your tiles
            // sit at slightly different pixel positions.
            //
            //   |  gap  | tile0 (ctrl hw) | gap | tile1 (video) |  gap  |
            //   |  gap  | tile2 (settings)| gap | tile3 (conn)  |  gap  |
            //
            // Tile size in the image ≈ 480×240, gap ≈ 16px, centred.
            // Top-left of grid ≈ x:148, y:120

            // Shared hotspot properties via a small component
            // Top-left tile — Control Hardware
            HotSpot {
                x: 150;  y: 80
                width: 450; height: 220
                onTapped: stackView.push(ctrlHwPage)
            }
            // Top-right tile — Video Control
            HotSpot {
                x: 620;  y: 80
                width: 450; height: 220
                onTapped: stackView.push(videoPage)
            }
            // Bottom-left tile — Settings
            HotSpot {
                x: 148;  y: 376
                width: 450; height: 220
                onTapped: stackView.push(settingsPage)
            }
            // Bottom-right tile — Connection
            HotSpot {
                x: 652;  y: 376
                width: 450; height: 220
                onTapped: stackView.push(connPage)
            }
        }
    }

    // ── SUB-PAGES ─────────────────────────────────────────────────────────
    Component { id: ctrlHwPage;   ControlHardwarePage { onBack: stackView.pop() } }
    Component { id: videoPage;    VideoControlPage    { onBack: stackView.pop() } }
    Component { id: settingsPage; SettingsPage        { onBack: stackView.pop() } }
    Component { id: connPage;     ConnectionPage      { onBack: stackView.pop() } }
}
