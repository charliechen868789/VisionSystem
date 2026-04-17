import QtQuick 2.12
import QtQuick.Controls 2.12
import QtMultimedia 5.12

BasePage {
    id: videoPage
    pageTitle: "CAMERA SELECT"
    onBack: { stackView.pop() }
    Camera {
        id: camera
        // Try to find Realtek first, otherwise use default
        deviceId: {
            var list = QtMultimedia.availableCameras;
            for (var i = 0; i < list.length; i++) {
                if (list[i].displayName.indexOf("Realtek") !== -1) {
                    return list[i].deviceId;
                }
            }
            return QtMultimedia.defaultCamera.deviceId;
        }
    }

    VideoOutput {
        anchors.fill: parent
        source: camera
        fillMode: VideoOutput.PreserveAspectCrop
    }

    // Camera Selection Menu
    Rectangle {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 20
        width: 300
        height: 50
        color: "#80000000" // Semi-transparent background
        radius: 10

        ComboBox {
            anchors.centerIn: parent
            width: 280
            model: QtMultimedia.availableCameras
            textRole: "displayName"
            currentIndex: 0

            onActivated: {
                camera.stop()
                camera.deviceId = model[index].deviceId
                camera.start()
            }
        }
    }

    // Diagnostic Log (Watch your console!)
    Component.onCompleted: {
        console.log("--- SYSTEM CAMERA LIST ---")
        QtMultimedia.availableCameras.forEach((cam, index) => {
            console.log(index + ": " + cam.displayName + " ID: " + cam.deviceId)
        })
    }
}
