import QtQuick 2.2
import QtQuick.Window 2.1
import CameraPlayer 1.0

Rectangle {
	width: Math.min(Screen.width, 800)
	height: Math.min(Screen.height, 256)

	CameraPlayer {
		width: 360
		height: 240
	}
}
