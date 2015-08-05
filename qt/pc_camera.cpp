#include "pc_camera.h"
#include "imagestream.h"

#include <QDebug>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

PCCamera::PCCamera(QObject *parent)
    : Camera(parent)
{
    m_image = new ImageStream(640, 480);
}

int PCCamera::initCapture()
{
    int err, i, w, h;
    int fd = open("/dev/video0", O_RDWR);
    if (fd < 0) {
        qWarning() << "open /dev/video0 fail " << fd;
        return fd;
    }
    videodev.fd = fd;

    struct v4l2_capability cap;
    if ((err = ioctl(fd, VIDIOC_QUERYCAP, &cap)) < 0) {
        qWarning() << "VIDIOC_QUERYCAP error " << err;
        goto err1;
    }
    qDebug() << "card =" << (char *)cap.card
             << " driver =" << (char *)cap.driver
             << " bus =" << (char *)cap.bus_info;

    if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
        qDebug() << "/dev/video0: Capable off capture";
    else {
        qWarning() << "/dev/video0: Not capable of capture";
        goto err1;
    }

    if (cap.capabilities & V4L2_CAP_STREAMING)
        qDebug() << "/dev/video0: Capable of streaming";
    else {
        qWarning() << "/dev/video0: Not capable of streaming";
        goto err1;
    }

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
        goto err1;
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
        goto err1;
    }

    struct v4l2_input input;
    memset(&input, 0, sizeof(input));
    input.index = 0;
    while ((err = ioctl(fd, VIDIOC_ENUMINPUT, &input)) == 0) {
        qDebug() << "input name =" << (char *)input.name
                 << " type =" << input.type
                 << " status =" << input.status
                 << " std =" << input.std;
        input.index++;
    }

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

    w = videodev.cap_width = fmt.fmt.pix.width;
    h = videodev.cap_height = fmt.fmt.pix.height;
    videodev.offset[0] = w * h;
    videodev.offset[1] = w*h*3/2;

    struct v4l2_requestbuffers reqbuf;
    reqbuf.count = 5;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    if ((err = ioctl(fd, VIDIOC_REQBUFS, &reqbuf)) < 0) {
        qWarning() << "Cannot allocate memory";
        goto err1;
    }
    qDebug() << "buffer actually allocated" << reqbuf.count;

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    for (i = 0; i < reqbuf.count; i++) {
        buf.type = reqbuf.type;
        buf.index = i;
        buf.memory = reqbuf.memory;
        Q_ASSERT(ioctl(fd, VIDIOC_QUERYBUF, &buf) == 0);

        videodev.buff_info[i].length = buf.length;
        videodev.buff_info[i].index = i;
        videodev.buff_info[i].start =
                (uchar *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        Q_ASSERT(videodev.buff_info[i].start != MAP_FAILED);

        memset((void *) videodev.buff_info[i].start, 0x80,
               videodev.buff_info[i].length);

        Q_ASSERT(ioctl(fd, VIDIOC_QBUF, &buf) == 0);
    }

    return 0;

err1:
    close(fd);
    return err;
}

void PCCamera::textureProcess(const uchar *data, int width, int height)
{
    m_image->yuyv2rgb(data, width, height);
}
