#include "camera_texture.h"

CameraTexture::CameraTexture(int width, int height)
	: mWidth(width), mHeight(height)
{
	mTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
	mTexture->setFormat(QOpenGLTexture::RGBFormat);
	mTexture->setSize(width, height);
	mTexture->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
	mTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
	mTexture->allocateStorage();

	setFiltering(QSGTexture::Nearest);
}

CameraTexture::~CameraTexture()
{
	delete mTexture;
}

void CameraTexture::updateFrame(void *data)
{
	mTexture->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt8, data);
}

