#ifndef CAMERA_PLAYER_H
#define CAMERA_PLAYER_H

#include <QQuickItem>
#include "camera_texture.h"
#include "sunxi_tvd_camera.h"

class CameraPlayer : public QQuickItem
{
	Q_OBJECT
	Q_PROPERTY(bool play READ play WRITE setPlay NOTIFY playChanged)
public:
	explicit CameraPlayer(QQuickItem *parent = 0);
	~CameraPlayer();

	QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);

	bool play() const { return mPlay; }
	void setPlay(bool);

signals:
	void playChanged(bool);

private:
	CameraTexture *mTexture;
	SunxiTVDCamera *mCamera;
	ImageStream mImage;
	bool mPlay;
};

#endif // CAMERA_PLAYER_H
