#include "camera_player.h"
#include "yuv2rgb_material.h"

CameraPlayer::CameraPlayer(QQuickItem *parent)
	: QQuickItem(parent), mTexture(NULL), mImage(360, 240),
	  mPlay(false), mBoundChanged(true), mTextureChanged(true)
{
	setFlag(ItemHasContents, true);
	connect(this, &CameraPlayer::xChanged, this, &CameraPlayer::boundChanged);
	connect(this, &CameraPlayer::yChanged, this, &CameraPlayer::boundChanged);
	connect(this, &CameraPlayer::widthChanged, this, &CameraPlayer::boundChanged);
	connect(this, &CameraPlayer::heightChanged, this, &CameraPlayer::boundChanged);

	mCamera = new SunxiTVDCamera(&mImage, this);
	connect(mCamera, &SunxiTVDCamera::imageChanged, this, &CameraPlayer::textureChanged);
	mCamera->start();
}

CameraPlayer::~CameraPlayer()
{
	delete mCamera;
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
	QSGGeometry *geometry;

	if (!mTexture)
		mTexture = new CameraTexture(360, 240);

	if (!oldNode) {
		node = new QSGGeometryNode;
		geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);
		geometry->setDrawingMode(GL_TRIANGLE_STRIP);
		node->setGeometry(geometry);
		node->setFlag(QSGNode::OwnsGeometry);

		YUV2RGBMaterial *material = new YUV2RGBMaterial;
		material->setTexture(mTexture);
		node->setMaterial(material);
		node->setFlag(QSGNode::OwnsMaterial);
	}
	else {
		node = static_cast<QSGGeometryNode *>(oldNode);
		geometry = node->geometry();
	}

	if (mBoundChanged) {
		QSGGeometry::TexturedPoint2D *vertices = geometry->vertexDataAsTexturedPoint2D();

		vertices[0].x = x();
		vertices[0].y = y() + height();
		vertices[0].tx = 0;
		vertices[0].ty = 1;

		vertices[1].x = x();
		vertices[1].y = y();
		vertices[1].tx = 0;
		vertices[1].ty = 0;

		vertices[2].x = x() + width();
		vertices[2].y = y() + height();
		vertices[2].tx = 1;
		vertices[2].ty = 1;

		vertices[3].x = x() + width();
		vertices[3].y = y();
		vertices[3].tx = 1;
		vertices[3].ty = 0;

		mBoundChanged = false;
		node->markDirty(QSGNode::DirtyGeometry);
	}

	if (mTextureChanged) {
		mTexture->updateFrame(mImage.getFrontImage());

		mTextureChanged = false;
		node->markDirty(QSGNode::DirtyMaterial);
	}

	return node;
}

