#ifndef CAMERA_PLAYER_H
#define CAMERA_PLAYER_H

#include <QQuickItem>
#include "camera_texture.h"
#include "sunxi_tvd_camera.h"

class CameraPlayer : public QQuickItem
{
	Q_OBJECT

public:
	explicit CameraPlayer(QQuickItem *parent = 0);
	~CameraPlayer();

	QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);

private:
	CameraTexture *mTexture;
	SunxiTVDCamera *mCamera;
	ImageStream mImage;
};

#endif // CAMERA_PLAYER_H
