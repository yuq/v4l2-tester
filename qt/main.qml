import QtQuick 2.2
import QtQuick.Window 2.1
import CameraPlayer 1.0

Rectangle {
    width: Math.min(Screen.width, 400)
	height: Math.min(Screen.height, 256)
	color: "black"

	CameraPlayer {
		id: camera
		width: 360
		height: 240
        play: true
	}

	Timer {
		interval: 10000
		repeat: true
        running: false
		onTriggered: camera.play = !camera.play
	}

    NumberAnimation {
        target: camera
        property: "width"
        from: 200
        to: 400
        duration: 1000
        easing.type: Easing.InOutQuad
        running: false
        loops: Animation.Infinite
    }
}
