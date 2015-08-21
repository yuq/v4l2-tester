#ifndef IMAGESTREAM_HPP
#define IMAGESTREAM_HPP

#include <QMutex>

class ImageStream
{
public:
    ImageStream(int width, int height);
    ~ImageStream();
	uchar *getFrontImage() { return data[front_index]; }
	uchar *getBackImage() { return data[(front_index + 1) % 2]; }
    void swapImage();
    bool isUpdated() { return updated; }
    void decUpdated() { updated--; }
    void lockFrontImage() { mutex.lock(); }
    void unlockFrontImage() { mutex.unlock(); }
    int getWidth() { return m_width; }
    int getHeight() { return m_height; }

    void yuv2rgb(const uchar *yuv, int yw, int yh);
    void yuyv2rgb(const uchar *yuv, int yw, int yh);
    void uyvy2rgb(const uchar *yuv, int yw, int yh);

private:
	uchar *data[2];
    int front_index;
    int updated;
    int m_width;
    int m_height;
    QMutex mutex;
};

#endif // IMAGESTREAM_HPP
