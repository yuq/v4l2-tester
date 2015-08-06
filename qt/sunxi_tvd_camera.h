#ifndef SUNXI_TVD_CAMERA_H
#define SUNXI_TVD_CAMERA_H

#include "camera.h"

class SunxiTVDCamera : public Camera
{
public:
    SunxiTVDCamera(QObject *parent = 0);

private:
    virtual int subInitCapture();
    virtual void textureProcess(const uchar *data, int width, int height);
};

#endif

