#include "pc_camera.h"
#include "imagestream.h"

#include <QDebug>

#include <errno.h>
#include <sys/ioctl.h>

PCCamera::PCCamera(QObject *parent)
    : Camera(parent)
{
    CAPTURE_DEVICE = "/dev/video0";
    m_image = new ImageStream(640, 480);
}

int PCCamera::subInitCapture()
{
    int err, fd = videodev.fd;

    struct v4l2_dbg_chip_info chip;
    if ((err = ioctl(fd, VIDIOC_DBG_G_CHIP_INFO, &chip)) < 0)
        qWarning() << "VIDIOC_DBG_G_CHIP_INFO error " << errno;
    else
        qDebug() << "chip info " << chip.name;

    bool support_fmt;
    struct v4l2_fmtdesc ffmt;
    memset(&ffmt, 0, sizeof(ffmt));
    ffmt.index = 0;
    ffmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    support_fmt = false;
    while ((err = ioctl(fd, VIDIOC_ENUM_FMT, &ffmt)) == 0) {
        qDebug() << "fmt" << ffmt.pixelformat << (char *)ffmt.description;
        if (ffmt.pixelformat == V4L2_PIX_FMT_YUYV)
            support_fmt = true;
        ffmt.index++;
    }
    if (!support_fmt) {
        qWarning() << "V4L2_PIX_FMT_YUYV is not supported by this camera";
        return -1;
    }

    bool support_640x480;
    struct v4l2_frmsizeenum fsize;
    memset(&fsize, 0, sizeof(fsize));
    fsize.index = 0;
    fsize.pixel_format = V4L2_PIX_FMT_YUYV;
    support_640x480 = false;
    while ((err = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsize)) == 0) {
        qDebug() << "frame size " << fsize.discrete.width << fsize.discrete.height;
        if (fsize.discrete.width == 640 && fsize.discrete.height == 480)
            support_640x480 = true;
        fsize.index++;
    }
    if (!support_640x480) {
        qWarning() << "frame size 640x480 is not supported by this camera";
        return -1;
    }

    vidioc_enuminput(fd);

    int index;
    if ((err = ioctl(fd, VIDIOC_G_INPUT, &index)) < 0)
        qWarning() << "VIDIOC_G_INPUT fail" << errno;
    else
        qDebug() << "current input index =" << index;

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if ((err = ioctl(fd, VIDIOC_G_FMT, &fmt)) < 0)
        qWarning() << "VIDIOC_G_FMT fail" << errno;
    else
        qDebug() << "fmt width =" << fmt.fmt.pix.width
                 << " height =" << fmt.fmt.pix.height
                 << " pfmt =" << fmt.fmt.pix.pixelformat;

    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    if ((err = ioctl(fd, VIDIOC_S_FMT, &fmt)) < 0)
        qWarning() << "VIDIOC_S_FMT fail" << errno;
    else
        qDebug() << "VIDIOC_S_FMT success";

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if ((err = ioctl(fd, VIDIOC_G_FMT, &fmt)) < 0)
        qWarning() << "VIDIOC_G_FMT fail" << errno;
    else
        qDebug() << "fmt width =" << fmt.fmt.pix.width
                 << " height =" << fmt.fmt.pix.height
                 << " pfmt =" << fmt.fmt.pix.pixelformat;
    Q_ASSERT(fmt.fmt.pix.width == 640);
    Q_ASSERT(fmt.fmt.pix.height == 480);
    Q_ASSERT(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV);

    videodev.cap_width = fmt.fmt.pix.width;
    videodev.cap_height = fmt.fmt.pix.height;

    return 0;
}

void PCCamera::textureProcess(const uchar *data, int width, int height)
{
    m_image->yuyv2rgb(data, width, height);
}
