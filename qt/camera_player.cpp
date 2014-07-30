#include "camera_player.h"
#include "yuv2rgb_material.h"
#include <QSGNode>

CameraPlayer::CameraPlayer(QQuickItem *parent)
	: QQuickItem(parent), mTexture(NULL), mImage(360, 240)
{
	setFlag(ItemHasContents, true);
	mCamera = new SunxiTVDCamera(&mImage, this);
	connect(mCamera, &SunxiTVDCamera::imageChanged, this, &CameraPlayer::update);
	mCamera->start();
}

CameraPlayer::~CameraPlayer()
{
	delete mCamera;
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
	}
	else {
		node = static_cast<QSGGeometryNode *>(oldNode);
		geometry = node->geometry();
	}

	mTexture->updateFrame(mImage.getFrontImage());
	node->markDirty(QSGNode::DirtyMaterial);

	return node;
}

