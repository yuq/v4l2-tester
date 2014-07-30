#include "imagestream.h"

ImageStream::ImageStream(int width, int height)
    : front_index(0), updated(false), m_width(width), m_height(height)
{
    for (int i = 0; i < 2; i++)
		data[i] = new uchar[width * height * 3];
}

ImageStream::~ImageStream()
{
    for (int i = 0; i < 2; i++)
		delete data[i];
}

void ImageStream::yuv2rgb(const uchar *yuv, int yw, int yh)
{
    int i, j;
    int w = m_width, h = m_height;
	uchar *rgb = getBackImage();

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
			rgb[i * w * 3 + j * 3 + 2] = yuv[yw * yh + i * yw + j * 2 + 1];
			rgb[i * w * 3 + j * 3 + 1] = yuv[yw * yh + i * yw + j * 2];
			rgb[i * w * 3 + j * 3 + 0] = yuv[i * yw * 2 + j * 2];
        }
    }
}

void ImageStream::swapImage()
{
    mutex.lock();
    front_index = (front_index + 1) % 2;
    updated = 4;
    mutex.unlock();
}
