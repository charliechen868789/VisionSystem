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
                list: backend.aiModelList
                selected: backend.activeAiModel
                onSelected_changed: function(idx) {
                    backend.setAiModel(idx)
                }
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
                            y: brightSlider.topPadding + brightSlider.availableHeight / 2 - height / 2
                            width: brightSlider.availableWidth
                            height: 4; radius: 2
                            color: "#1a2540"
                            Rectangle {
                                width: brightSlider.visualPosition * parent.width
                                height: parent.height; radius: 2
                                color: "#9b3de8"
                            }
                        }
                        handle: Rectangle {
                            x: brightSlider.leftPadding + brightSlider.visualPosition
                               * (brightSlider.availableWidth - width)
                            y: brightSlider.topPadding + brightSlider.availableHeight / 2 - height / 2
                            width: 16; height: 16; radius: 8
                            color: "#c0d0e8"
                            border.color: "#9b3de8"
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