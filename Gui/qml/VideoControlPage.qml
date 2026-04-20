import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

BasePage {
    id: videoSetup
    pageTitle: "VIDEO SETUP"
    accentColor: "#9b3de8"

    signal startVideo()
    
    onStartVideo: {
        stackView.push("qrc:/qml/VideoPreviewPage.qml")
    }
    // ── Reusable toggle component ─────────────────────────────────────────────
    component ToggleRow: Item {
        id: toggleRoot
        width: parent.width
        height: 54
        property string label: ""
        property bool value: false
        signal toggled(bool newVal)

        Rectangle {
            anchors.fill: parent
            radius: 8
            color: "#0d1530"
            border.color: "#1e3050"

            Text {
                anchors.verticalCenter: parent.verticalCenter
                leftPadding: 20
                text: toggleRoot.label
                color: "#c0d0e8"
                font.pixelSize: 16
                renderType: Text.NativeRendering
            }

            Rectangle {
                id: track
                width: 42; height: 22; radius: 11
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 20
                color: toggleRoot.value ? "#9b3de8" : "#1a2540"
                border.color: "#1e3050"

                Behavior on color { ColorAnimation { duration: 120 } }

                Rectangle {
                    width: 18; height: 18; radius: 9
                    y: 2
                    x: toggleRoot.value ? 22 : 2
                    color: "#c0d0e8"
                    Behavior on x { NumberAnimation { duration: 120 } }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        toggleRoot.value = !toggleRoot.value
                        toggleRoot.toggled(toggleRoot.value)
                    }
                }
            }
        }
    }

    // ── Reusable dropdown component ───────────────────────────────────────────
    component DropdownRow: Item {
        id: dropRoot
        width: parent.width
        height: open ? 54 + list.length * 42 : 54
        property string label: ""
        property var list: []
        property int selected: 0
        property bool open: false
        signal selected_changed(int idx)

        Behavior on height { NumberAnimation { duration: 150 } }

        Rectangle {
            id: dropHeader
            width: parent.width
            height: 54
            radius: 8
            color: dropRoot.open ? "#12253d" : "#0d1530"
            border.color: dropRoot.open ? "#9b3de8" : "#1e3050"

            Behavior on border.color { ColorAnimation { duration: 120 } }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                leftPadding: 20
                text: dropRoot.label + ": " + dropRoot.list[dropRoot.selected]
                color: "#c0d0e8"
                font.pixelSize: 16
                renderType: Text.NativeRendering
            }

            // Chevron
            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 20
                text: dropRoot.open ? "▲" : "▼"
                color: "#9b3de8"
                font.pixelSize: 12
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: dropRoot.open = !dropRoot.open
            }
        }

        Column {
            y: 58
            width: parent.width
            spacing: 2
            visible: dropRoot.open
            clip: true

            Repeater {
                model: dropRoot.list
                Rectangle {
                    width: dropRoot.width
                    height: 40
                    radius: 6
                    color: dropItemMouse.containsMouse ? "#12253d" : "#0a1128"
                    border.color: dropRoot.selected === index ? "#9b3de8" : "#1e3050"

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        leftPadding: 20
                        text: modelData
                        color: dropRoot.selected === index ? "#9b3de8" : "#c0d0e8"
                        font.pixelSize: 15
                        renderType: Text.NativeRendering
                    }

                    // Checkmark for selected
                    Text {
                        visible: dropRoot.selected === index
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 16
                        text: "✓"
                        color: "#9b3de8"
                        font.pixelSize: 14
                    }

                    MouseArea {
                        id: dropItemMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            dropRoot.selected = index
                            dropRoot.open = false
                            dropRoot.selected_changed(index)
                        }
                    }
                }
            }
        }
    }

    // ── Section label helper ──────────────────────────────────────────────────
    component SectionLabel: Text {
        color: "#6a7fa8"
        font.pixelSize: 11
        font.letterSpacing: 2
        font.weight: Font.Medium
        renderType: Text.NativeRendering
        topPadding: 6
    }

    // ── Scrollable content ────────────────────────────────────────────────────
    Flickable {
        anchors { fill: parent; margins: 40 }
        contentHeight: mainCol.implicitHeight
        clip: true
        flickableDirection: Flickable.VerticalFlick

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            contentItem: Rectangle {
                implicitWidth: 4
                radius: 2
                color: "#9b3de8"
                opacity: 0.6
            }
        }

        Column {
            id: mainCol
            width: parent.width
            spacing: 8

            // ─────────────────────────────────────────────────────────────────
            // SECTION: AI
            // ─────────────────────────────────────────────────────────────────
            SectionLabel { text: "AI  PIPELINE" }

            DropdownRow {
                label: "AI Model"
                list: ["General Model", "Fast AI", "High Accuracy AI",
                       "Face Optimized", "Edge Lite", "Custom YOLO",
                       "Pose Estimation"]
                selected: backend.aiModel
                onSelected_changed: function(idx) { backend.aiModel = idx }
            }

            // Confidence threshold slider
            Item {
                width: parent.width
                height: 54
                Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: "#0d1530"
                    border.color: "#1e3050"

                    Text {
                        id: confLabel
                        anchors.verticalCenter: parent.verticalCenter
                        leftPadding: 20
                        text: "Confidence: " + Math.round(confSlider.value * 100) + "%"
                        color: "#c0d0e8"
                        font.pixelSize: 16
                        renderType: Text.NativeRendering
                    }

                    Slider {
                        id: confSlider
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 20
                        width: 140
                        from: 0.1; to: 1.0; stepSize: 0.05
                        value: backend.aiConfidence

                        background: Rectangle {
                            x: confSlider.leftPadding
                            y: confSlider.topPadding + confSlider.availableHeight / 2 - height / 2
                            width: confSlider.availableWidth
                            height: 4; radius: 2
                            color: "#1a2540"
                            Rectangle {
                                width: confSlider.visualPosition * parent.width
                                height: parent.height; radius: 2
                                color: "#9b3de8"
                            }
                        }
                        handle: Rectangle {
                            x: confSlider.leftPadding + confSlider.visualPosition
                               * (confSlider.availableWidth - width)
                            y: confSlider.topPadding + confSlider.availableHeight / 2 - height / 2
                            width: 16; height: 16; radius: 8
                            color: "#c0d0e8"
                            border.color: "#9b3de8"
                        }
                        onMoved: backend.aiConfidence = value
                    }
                }
            }

            ToggleRow {
                label: "Object Detection"
                value: backend.objectDetection
                onToggled: function(v) { backend.objectDetection = v }
            }

            ToggleRow {
                label: "Face Detection"
                value: backend.faceDetection
                onToggled: function(v) { backend.faceDetection = v }
            }

            ToggleRow {
                label: "Object Tracking"
                value: backend.trackingEnabled
                onToggled: function(v) { backend.trackingEnabled = v }
            }

            ToggleRow {
                label: "Pose Estimation"
                value: backend.poseEstimation
                onToggled: function(v) { backend.poseEstimation = v }
            }

            ToggleRow {
                label: "Anomaly Detection"
                value: backend.anomalyDetection
                onToggled: function(v) { backend.anomalyDetection = v }
            }

            // ─────────────────────────────────────────────────────────────────
            // SECTION: VIDEO SOURCE
            // ─────────────────────────────────────────────────────────────────
            SectionLabel { text: "VIDEO  SOURCE" }

            DropdownRow {
                label: "Camera"
                list: backend.cameraList
                selected: backend.activeCamera
                onSelected_changed: function(idx) {
                    backend.switchCamera(idx)
                }
            }

            DropdownRow {
                label: "Resolution"
                list: ["3840×2160  4K", "1920×1080  FHD", "1280×720  HD",
                       "640×480  VGA", "320×240  QVGA"]
                selected: backend.resolution
                onSelected_changed: function(idx) { backend.resolution = idx }
            }

            DropdownRow {
                label: "Frame Rate"
                list: ["60 fps", "30 fps", "24 fps", "15 fps", "10 fps"]
                selected: backend.frameRate
                onSelected_changed: function(idx) { backend.frameRate = idx }
            }

            // ─────────────────────────────────────────────────────────────────
            // SECTION: IMAGE
            // ─────────────────────────────────────────────────────────────────
            SectionLabel { text: "IMAGE  SETTINGS" }

            // Brightness slider
            Item {
                width: parent.width
                height: 54
                Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: "#0d1530"
                    border.color: "#1e3050"
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        leftPadding: 20
                        text: "Brightness: " + brightSlider.value + "%"
                        color: "#c0d0e8"
                        font.pixelSize: 16
                        renderType: Text.NativeRendering
                    }
                    Slider {
                        id: brightSlider
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 20
                        width: 140
                        from: 0; to: 100; stepSize: 1
                        value: backend.brightness
                        background: Rectangle {
                            x: brightSlider.leftPadding
                            y: brightSlider.topPadding + brightSlider.availableHeight/2 - height/2
                            width: brightSlider.availableWidth
                            height: 4; radius: 2; color: "#1a2540"
                            Rectangle {
                                width: brightSlider.visualPosition * parent.width
                                height: parent.height; radius: 2; color: "#9b3de8"
                            }
                        }
                        handle: Rectangle {
                            x: brightSlider.leftPadding + brightSlider.visualPosition
                               * (brightSlider.availableWidth - width)
                            y: brightSlider.topPadding + brightSlider.availableHeight/2 - height/2
                            width: 16; height: 16; radius: 8
                            color: "#c0d0e8"; border.color: "#9b3de8"
                        }
                        onMoved: backend.brightness = value
                    }
                }
            }

            ToggleRow {
                label: "Night Mode"
                value: backend.nightMode
                onToggled: function(v) { backend.nightMode = v }
            }

            ToggleRow {
                label: "Flip Horizontal"
                value: backend.flipHorizontal
                onToggled: function(v) { backend.flipHorizontal = v }
            }

            ToggleRow {
                label: "Flip Vertical"
                value: backend.flipVertical
                onToggled: function(v) { backend.flipVertical = v }
            }

            // ─────────────────────────────────────────────────────────────────
            // SECTION: OUTPUT
            // ─────────────────────────────────────────────────────────────────
            SectionLabel { text: "OUTPUT" }

            ToggleRow {
                label: "Record to File"
                value: backend.recordToFile
                onToggled: function(v) { backend.recordToFile = v }
            }

            ToggleRow {
                label: "RTSP Stream Out"
                value: backend.rtspOut
                onToggled: function(v) { backend.rtspOut = v }
            }

            ToggleRow {
                label: "Show Overlays"
                value: backend.showOverlays
                onToggled: function(v) { backend.showOverlays = v }
            }

            // ─────────────────────────────────────────────────────────────────
            // START BUTTON
            // ─────────────────────────────────────────────────────────────────
            Item { height: 12 }

            Rectangle {
                Layout.fillWidth: true
                width: parent.width
                height: 54
                radius: 8
                color: startMouse.containsMouse ? "#b060f0" : "#9b3de8"

                Behavior on color { ColorAnimation { duration: 120 } }

                MouseArea {
                    id: startMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: videoSetup.startVideo()
                }

                Text {
                    anchors.centerIn: parent
                    text: "▶   START VIDEO"
                    color: "white"
                    font.pixelSize: 16
                    font.weight: Font.Medium
                    renderType: Text.NativeRendering
                }
            }

            Item { height: 20 }
        }
    }
}
