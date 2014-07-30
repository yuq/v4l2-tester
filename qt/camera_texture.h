#ifndef CAMERA_TEXTURE_H
#define CAMERA_TEXTURE_H

#include <QSGTexture>
#include <QOpenGLTexture>

class CameraTexture : public QSGTexture
{
public:
	CameraTexture(int width, int height);
	virtual ~CameraTexture();

	virtual void bind() { mTexture->bind(); }
	virtual bool hasAlphaChannel() const { return false; }
	virtual bool hasMipmaps() const { return false; }
	virtual int textureId() const { return mTexture->textureId(); }
	virtual QSize textureSize() const { return QSize(mWidth, mHeight); }

	void updateFrame(void *data);

private:
	int mWidth;
	int mHeight;
	QOpenGLTexture *mTexture;
};

#endif // CAMERA_TEXTURE_H
