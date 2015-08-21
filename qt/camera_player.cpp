#include "camera_player.h"
#include <QSGNode>

#ifdef CONFIG_SUNXI_PLATFORM
#include "sunxi_tvd_camera.h"
#endif
#ifdef CONFIG_PC_PLATFORM
#include "pc_camera.h"
#endif
#ifdef CONFIG_IMX_PLATFORM
#include "imx_tvd_camera.h"
#endif

CameraPlayer::CameraPlayer(QQuickItem *parent)
    : QQuickItem(parent), mPlay(false), mBoundChanged(true), mTextureChanged(true)
{
	setFlag(ItemHasContents, true);
	connect(this, &CameraPlayer::xChanged, this, &CameraPlayer::boundChanged);
	connect(this, &CameraPlayer::yChanged, this, &CameraPlayer::boundChanged);
	connect(this, &CameraPlayer::widthChanged, this, &CameraPlayer::boundChanged);
	connect(this, &CameraPlayer::heightChanged, this, &CameraPlayer::boundChanged);

#ifdef CONFIG_SUNXI_PLATFORM
    mCamera = new SunxiTVDCamera(this);
#endif
#ifdef CONFIG_PC_PLATFORM
    mCamera = new PCCamera(this);
#endif
#ifdef CONFIG_IMX_PLATFORM
    mCamera = new IMXTVDCamera(this);
#endif
    connect(mCamera, &Camera::imageChanged, this, &CameraPlayer::textureChanged);
	mCamera->start();
}

CameraPlayer::~CameraPlayer()
{

}

void CameraPlayer::setPlay(bool value)
{
	if (mPlay == value)
		return;

	if (value)
		mCamera->startStream();
	else
		mCamera->stopStream();

	mPlay = value;
	emit playChanged(value);
}

void CameraPlayer::boundChanged()
{
	mBoundChanged = true;
	update();
}

void CameraPlayer::textureChanged()
{
	mTextureChanged = true;
	update();
}

QSGNode *CameraPlayer::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
	QSGGeometryNode *node;

    if (!oldNode)
        node = mCamera->createNode();
    else
		node = static_cast<QSGGeometryNode *>(oldNode);

	if (mBoundChanged) {
        mCamera->updateGeometry(x(), y(), width(), height());
		mBoundChanged = false;
		node->markDirty(QSGNode::DirtyGeometry);
	}

	if (mTextureChanged) {
        mCamera->updateMaterial();
		mTextureChanged = false;
		node->markDirty(QSGNode::DirtyMaterial);
	}

	return node;
}

