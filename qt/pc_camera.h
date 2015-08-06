#ifndef PC_CAMERA_H
#define PC_CAMERA_H

#include "camera.h"

class PCCamera : public Camera
{
public:
    PCCamera(QObject *parent = 0);

private:
    virtual int subInitCapture();
    virtual void textureProcess(const uchar *data, int width, int height);
};

#endif
