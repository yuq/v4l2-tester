#include "sunxi_tvd_camera.h"
#include "imagestream.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

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

/* device to be used for capture */
#define CAPTURE_DEVICE      "/dev/video1"
#define CAPTURE_NAME		"Capture"


SunxiTVDCamera::SunxiTVDCamera(QObject *parent)
    : Camera(parent)
{
    m_image = new ImageStream(360, 240);
}

int SunxiTVDCamera::initCapture()
{
	int fd;
	struct v4l2_format fmt;

	int ret, i, w, h;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_buffer buf;
	struct v4l2_capability capability;

	if (videodev.fd > 0)
		return 0;

	/* Open the capture device */
	fd = open(CAPTURE_DEVICE, O_RDWR);

	if (fd <= 0) {
		qDebug() << "Cannot open " << CAPTURE_DEVICE;
		return -1;
	}
	videodev.fd = fd;

	/* Check if the device is capable of streaming */
	if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
		qDebug() << "VIDIOC_QUERYCAP error";
		goto ERROR;
	}

	if (capability.capabilities & V4L2_CAP_STREAMING)
		qDebug() << CAPTURE_NAME << ": Capable of streaming";
	else {
		qDebug() << CAPTURE_NAME << ": Not capable of streaming";
		goto ERROR;
	}

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
	if (-1 == ioctl (fd, VIDIOC_S_FMT, &fmt)){
		qDebug() << "VIDIOC_S_FMT error!";
		return -1;
	}

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		qDebug() << "VIDIOC_G_FMT error";
		goto ERROR;
	}

	w = videodev.cap_width = fmt.fmt.pix.width;
	h = videodev.cap_height = fmt.fmt.pix.height;
	videodev.offset[0] = w * h;

	switch (fmt.fmt.pix.pixelformat) {
	case V4L2_PIX_FMT_YUV422P:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
		videodev.offset[1] = w*h*3/2;
		break;
	case V4L2_PIX_FMT_YUV420:
		videodev.offset[1] = w*h*5/4;
		break;
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_HM12:
		videodev.offset[1] = videodev.offset[0];
		break;
	default:
		qDebug() << "csi_format is not found!";
		break;
	}

	qDebug() << "cap size " << fmt.fmt.pix.width << " x " <<
				fmt.fmt.pix.height << " offset: " << videodev.offset[0] <<
				", " << videodev.offset[1];

	/* Buffer allocation
	 * Buffer can be allocated either from capture driver or
	 * user pointer can be used
	 */
	/* Request for MAX_BUFFER input buffers. As far as Physically contiguous
	 * memory is available, driver can allocate as many buffers as
	 * possible. If memory is not available, it returns number of
	 * buffers it has allocated in count member of reqbuf.
	 * HERE count = number of buffer to be allocated.
	 * type = type of device for which buffers are to be allocated.
	 * memory = type of the buffers requested i.e. driver allocated or
	 * user pointer
	 */
	reqbuf.count = CAPTURE_MAX_BUFFER;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
	if (ret < 0) {
		qDebug() << "Cannot allocate memory";
		goto ERROR;
	}
	/* Store the number of buffers actually allocated */
	videodev.numbuffer = reqbuf.count;
	qDebug() << CAPTURE_NAME << ": Number of requested buffers = " <<
				videodev.numbuffer;

	memset(&buf, 0, sizeof(buf));

	/* Mmap the buffers
	 * To access driver allocated buffer in application space, they have
	 * to be mmapped in the application space using mmap system call */
	for (i = 0; i < videodev.numbuffer; i++) {
		buf.type = reqbuf.type;
		buf.index = i;
		buf.memory = reqbuf.memory;
		ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
		if (ret < 0) {
			qDebug() << "VIDIOC_QUERYCAP error";
			videodev.numbuffer = i;
			goto ERROR;
		}

		videodev.buff_info[i].length = buf.length;
		videodev.buff_info[i].index = i;
		videodev.buff_info[i].start =
				(uchar *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

		if (videodev.buff_info[i].start == MAP_FAILED) {
			qDebug() << "Cannot mmap = " << i << " buffer";
			videodev.numbuffer = i;
			goto ERROR;
		}

		memset((void *) videodev.buff_info[i].start, 0x80,
			   videodev.buff_info[i].length);
		/* Enqueue buffers
		 * Before starting streaming, all the buffers needs to be
		 * en-queued in the driver incoming queue. These buffers will
		 * be used by thedrive for storing captured frames.
		 */
		ret = ioctl(fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			qDebug() << "VIDIOC_QBUF error";
			videodev.numbuffer = i + 1;
			goto ERROR;
		}
	}

	qDebug() << CAPTURE_NAME << ": Init done successfully";
	return 0;

ERROR:
	closeCapture();
	return -1;
}

void SunxiTVDCamera::textureProcess(const uchar *data, int width, int height)
{
	m_image->yuv2rgb(data, width, height);
}
