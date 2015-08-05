#ifndef CAMERA_PLAYER_H
#define CAMERA_PLAYER_H

#include <QQuickItem>

class Camera;

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

private slots:
	void textureChanged();
	void boundChanged();

private:
    Camera *mCamera;
	bool mPlay;
	bool mBoundChanged;
	bool mTextureChanged;
};

#endif // CAMERA_PLAYER_H
