#ifndef CAMERA_TEXTURE_H
#define CAMERA_TEXTURE_H

#include <QSGTexture>
#include <QOpenGLTexture>

class CameraTexture : public QSGTexture
{
public:
	CameraTexture(int width, int height);
	virtual ~CameraTexture();

	virtual void bind() { glBindTexture(GL_TEXTURE_2D, mTexture); }
	virtual bool hasAlphaChannel() const { return false; }
	virtual bool hasMipmaps() const { return false; }
	virtual int textureId() const { return mTexture; }
	virtual QSize textureSize() const { return QSize(mWidth, mHeight); }

	void updateFrame(void *data);

private:
	int mWidth;
	int mHeight;
	GLuint mTexture;
};

#endif // CAMERA_TEXTURE_H
