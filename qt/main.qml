import QtQuick 2.2
import QtQuick.Window 2.1
import CameraPlayer 1.0

Rectangle {
	width: Math.min(Screen.width, 800)
	height: Math.min(Screen.height, 256)

	CameraPlayer {
		id: camera
		width: 360
		height: 240
	}

	Timer {
		interval: 10000
		repeat: true
		running: true
		onTriggered: camera.play = !camera.play
	}
}
