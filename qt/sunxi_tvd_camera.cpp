#include "sunxi_tvd_camera.h"
#include "imagestream.h"

#include <sys/ioctl.h>

#include <QDebug>

extern "C" {
#include <video/sunxi_disp_ioctl.h>
}

#define VIN_SYSTEM_NTSC	0
#define VIN_SYSTEM_PAL	1

#define VIN_ROW_NUM	1
#define VIN_COL_NUM	1

#define CONFIG_VIDEO_STREAM_NTSC

#ifdef CONFIG_VIDEO_STREAM_NTSC
#define VIN_SYSTEM	VIN_SYSTEM_NTSC
#else
#define VIN_SYSTEM	VIN_SYSTEM_PAL
#endif


SunxiTVDCamera::SunxiTVDCamera(QObject *parent)
    : Camera(parent)
{
    CAPTURE_DEVICE = "/dev/video1";
    m_image = new ImageStream(360, 240);
}

int SunxiTVDCamera::subInitCapture()
{
    int ret, fd = videodev.fd;

    struct v4l2_format fmt;
	//set position and auto calculate size
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_PRIVATE;
	fmt.fmt.raw_data[0] =0;//interface
	fmt.fmt.raw_data[1] =VIN_SYSTEM;//system, 1=pal, 0=ntsc
	fmt.fmt.raw_data[8] =VIN_ROW_NUM;//row
	fmt.fmt.raw_data[9] =VIN_COL_NUM;//column
	fmt.fmt.raw_data[10] =0;//channel_index
	fmt.fmt.raw_data[11] =1;//channel_index
	fmt.fmt.raw_data[12] =0;//channel_index
	fmt.fmt.raw_data[13] =0;//channel_index
    if ((ret = ioctl (fd, VIDIOC_S_FMT, &fmt)) < 0) {
		qDebug() << "VIDIOC_S_FMT error!";
		return -1;
	}

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if ((ret = ioctl(fd, VIDIOC_G_FMT, &fmt)) < 0) {
		qDebug() << "VIDIOC_G_FMT error";
        return -1;
	}

    videodev.cap_width = fmt.fmt.pix.width;
    videodev.cap_height = fmt.fmt.pix.height;

	qDebug() << "cap size " << fmt.fmt.pix.width << " x " <<
                fmt.fmt.pix.height;

    qDebug() << "Init done successfully";
	return 0;
}

void SunxiTVDCamera::textureProcess(const uchar *data, int width, int height)
{
	m_image->yuv2rgb(data, width, height);
}
