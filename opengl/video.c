/*
 * video.c
 *
 *	Copyright (C) 2007, UP-TECH Corporation
 *
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pthread.h>

#include <linux/fb.h>
#include <linux/videodev2.h>

#include "video.h"
#include "display.h"
#include <video/sunxi_disp_ioctl.h>

#define VIN_SYSTEM_NTSC	0
#define VIN_SYSTEM_PAL	1

#define VIN_ROW_NUM	1
#define VIN_COL_NUM	1
#define VIN_SYSTEM	VIN_SYSTEM_NTSC

//#define LOGTIME

#define DEFAULT_FB_DEVICE	"/dev/fb0"

static unsigned int frame_count = 0;
const static int frame_devisor = 3;

#define DEBUG
#ifdef DEBUG
#define DPRINTF(fmt, x...)	fprintf(logfile, "Debug:"fmt, ## x);
#else
#define DPRINTF(fmt, x...)
#endif

#define LOG(fmt, x...)	if(logfile) fprintf(logfile, fmt, ## x)
static FILE *logfile=NULL;

#define min(a,b)	((a)<(b)?(a):(b))
#define max(a,b)	((a)>(b)?(a):(b))

/* structure used to store information of the buffers */
struct buf_info{
	int index;
	unsigned int length;
	char *start;
};

#define CAPTURE_MAX_BUFFER		5

/* device to be used for capture */
#define CAPTURE_DEVICE		"/dev/video1"
#define CAPTURE_NAME		"Capture"

#define DEF_PIX_FMT		V4L2_PIX_FMT_YUV420	//V4L2_PIX_FMT_NV12

struct fb_info
{
	int fb_width, fb_height, fb_line_len, fb_size;
	int fb_bpp;
};
static struct fb_info fbinfo;

struct video_dev
{
	int fd;	//for video device
	unsigned int offset[2];	//capture data C offset
	int cap_width, cap_height;

	struct v4l2_buffer capture_buf;
	struct buf_info buff_info[CAPTURE_MAX_BUFFER];
	int numbuffer;

};
static struct video_dev videodev={.fd=-1, };

static void video_close(struct video_dev *pvdev)
{
	int i;
	struct buf_info *buff_info;

	/* Un-map the buffers */
	for (i = 0; i < CAPTURE_MAX_BUFFER; i++){
		buff_info = &pvdev->buff_info[i];
		if(buff_info->start){
			munmap(buff_info->start, buff_info->length);
			buff_info->start = NULL;
		}
	}

	if(pvdev->fd>=0){
		close(pvdev->fd);
		pvdev->fd = -1;
	}
}

static void print_status(int fd)
{
	struct v4l2_format fmt_priv;
	memset(&fmt_priv, 0, sizeof(fmt_priv));
	fmt_priv.type                = V4L2_BUF_TYPE_PRIVATE;
	if (-1 == ioctl (fd, VIDIOC_G_FMT, &fmt_priv)) //设置自定义
	{
		printf("VIDIOC_G_FMT error!  a\n");
		return;
	}
	printf("interface=%d\n", fmt_priv.fmt.raw_data[0]);
	printf("system=%d\n", fmt_priv.fmt.raw_data[1]);
	printf("format=%d\n", fmt_priv.fmt.raw_data[2]);
	printf("row=%d\n", fmt_priv.fmt.raw_data[8]);
	printf("column=%d\n", fmt_priv.fmt.raw_data[9]);
	printf("channel_index[0]=%d\n", fmt_priv.fmt.raw_data[10]);
	printf("channel_index[1]=%d\n", fmt_priv.fmt.raw_data[11]);
	printf("channel_index[2]=%d\n", fmt_priv.fmt.raw_data[12]);
	printf("channel_index[3]=%d\n", fmt_priv.fmt.raw_data[13]);
	printf("status[0]=%d\n", fmt_priv.fmt.raw_data[16]);
	printf("status[1]=%d\n", fmt_priv.fmt.raw_data[17]);
	printf("status[2]=%d\n", fmt_priv.fmt.raw_data[18]);
	printf("status[3]=%d\n", fmt_priv.fmt.raw_data[19]);
}

/*===============================initCapture==================================*/
/* This function initializes capture device. It selects an active input
 * and detects the standard on that input. It then allocates buffers in the
 * driver's memory space and mmaps them in the application space.
 */
static int initCapture(const char *dev, struct video_dev *pvdev)
{
	int fd;
	struct v4l2_format fmt;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;

	int ret, i, w, h;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_buffer buf;
	struct v4l2_capability capability;
	int index;

	/* Open the capture device */
	fd	= open(dev, O_RDWR);
	DPRINTF("%s, %d\n", __FUNCTION__, __LINE__);

	if (fd	<= 0) {
		DPRINTF("%s, %d\n", __FUNCTION__, __LINE__);
		LOG("Cannot open = %s device\n", CAPTURE_DEVICE);
		return -1;
	}
	pvdev->fd = fd;

	/* Check if the device is capable of streaming */
	if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
		LOG("VIDIOC_QUERYCAP error\n");
		goto ERROR;
	}

	if (capability.capabilities & V4L2_CAP_STREAMING){
		LOG("%s: Capable of streaming\n", CAPTURE_NAME);
	}
	else {
		LOG("%s: Not capable of streaming\n", CAPTURE_NAME);
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
		LOG("VIDIOC_S_FMT error!\n");
		return -1; 
	}

	usleep(100000);
	print_status(fd);

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		LOG("VIDIOC_G_FMT error\n");
		goto ERROR;
	}

	w = pvdev->cap_width = fmt.fmt.pix.width;
	h = pvdev->cap_height = fmt.fmt.pix.height;
	pvdev->offset[0] = w * h;

	switch(fmt.fmt.pix.pixelformat){
	case V4L2_PIX_FMT_YUV422P:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
		pvdev->offset[1] = w*h*3/2;
		break;
	case V4L2_PIX_FMT_YUV420:
		pvdev->offset[1] = w*h*5/4;
		break;
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_HM12:
		pvdev->offset[1] = pvdev->offset[0];
		break;

	default:
		LOG("csi_format is not found!\n");
		break;

	}

	LOG("cap size %d x %d offset: %x, %x pixelformat: =========%x==========%x=========%x=========\n", 
		fmt.fmt.pix.width, fmt.fmt.pix.height, pvdev->offset[0], pvdev->offset[1], V4L2_PIX_FMT_YUV420, V4L2_PIX_FMT_NV12, fmt.fmt.pix.pixelformat);

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
	 * user pointer */
	reqbuf.count = CAPTURE_MAX_BUFFER;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
	if (ret < 0) {
		LOG("Cannot allocate memory\n");
		goto ERROR;
	}
	/* Store the number of buffers actually allocated */
	pvdev->numbuffer = reqbuf.count;
	LOG("%s: Number of requested buffers = %d\n", CAPTURE_NAME,
			pvdev->numbuffer);

	memset(&buf, 0, sizeof(buf));

	/* Mmap the buffers
	 * To access driver allocated buffer in application space, they have
	 * to be mmapped in the application space using mmap system call */
	for (i = 0; i < (pvdev->numbuffer); i++) {
		buf.type = reqbuf.type;
		buf.index = i;
		buf.memory = reqbuf.memory;
		ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
		if (ret < 0) {
			LOG("VIDIOC_QUERYCAP error\n");
			pvdev->numbuffer = i;
			goto ERROR;
		}

		pvdev->buff_info[i].length = buf.length;
		pvdev->buff_info[i].index = i;
		pvdev->buff_info[i].start = mmap(NULL, buf.length,
				PROT_READ | PROT_WRITE, MAP_SHARED, fd,
				buf.m.offset);

		if (pvdev->buff_info[i].start == MAP_FAILED) {
			LOG("Cannot mmap = %d buffer\n", i);
			pvdev->numbuffer = i;
			goto ERROR;
		}

		memset((void *) pvdev->buff_info[i].start, 0x80,
				pvdev->buff_info[i].length);
		/* Enqueue buffers
		 * Before starting streaming, all the buffers needs to be
		 * en-queued in the driver incoming queue. These buffers will
		 * be used by thedrive for storing captured frames. */
		ret = ioctl(fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			LOG("VIDIOC_QBUF error\n");
			pvdev->numbuffer = i + 1;
			goto ERROR;
		}
	}

	LOG("%s: Init done successfully\n\n", CAPTURE_NAME);
	return 0;

ERROR:
	video_close(pvdev);

	return -1;
}

static int Getfb_info(char *dev)
{
	int fb;
	struct fb_var_screeninfo fb_vinfo;
	struct fb_fix_screeninfo fb_finfo;
	char* fb_dev_name=NULL;
	
	if (!(fb_dev_name = getenv("FRAMEBUFFER")))
		fb_dev_name=dev;

	fb = open (fb_dev_name, O_RDWR);
	if(fb<0){
		DPRINTF("device %s open failed\n", fb_dev_name);
		return -1;
	}
	
	if (ioctl(fb, FBIOGET_VSCREENINFO, &fb_vinfo)) {
		DPRINTF("Can't get VSCREENINFO: %s\n");
		close(fb);
		return -1;
	}

	if (ioctl(fb, FBIOGET_FSCREENINFO, &fb_finfo)) {
		DPRINTF("Can't get FSCREENINFO: %s\n");
		return -1;
	}

	fbinfo.fb_bpp= fb_vinfo.red.length + fb_vinfo.green.length +
		fb_vinfo.blue.length + fb_vinfo.transp.length;

	fbinfo.fb_width = fb_vinfo.xres;
	fbinfo.fb_height = fb_vinfo.yres;
	fbinfo.fb_line_len = fb_finfo.line_length;
	fbinfo.fb_size = fb_finfo.smem_len;

	DPRINTF("frame buffer: %d(%d)x%d,  %dbpp, 0x%xbyte\n", 
		fbinfo.fb_width, fbinfo.fb_line_len, fbinfo.fb_height, fbinfo.fb_bpp, fbinfo.fb_size);
		
	close(fb);

	return 0;
}

static int video_start_cap(struct video_dev *pvdev)
{
	int a, ret;

	/* run section
	 * STEP2:
	 * Here display and capture channels are started for streaming. After
	 * this capture device will start capture frames into enqueued
	 * buffers and display device will start displaying buffers from
	 * the qneueued buffers */

	/* Start Streaming. on capture device */
	a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(pvdev->fd, VIDIOC_STREAMON, &a);
	if (ret < 0) {
		LOG("capture VIDIOC_STREAMON error fd=%d\n", pvdev->fd);
		return ret;
	}
	LOG("%s: Stream on...\n", CAPTURE_NAME);

	/* Set the capture buffers for queuing and dqueueing operation */
	pvdev->capture_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	pvdev->capture_buf.index = 0;
	pvdev->capture_buf.memory = V4L2_MEMORY_MMAP;

	return 0;
}

static inline int video_cap_frame(struct video_dev *pvdev)
{
	int ret;
	struct v4l2_buffer buf;

	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_USERPTR;

#ifdef LOGTIME
	struct timeval btime, ctime, ltime;

	gettimeofday( &btime, NULL );
	ltime = btime;
	LOG("Begin\n");
#endif

	pthread_testcancel();
	/* Dequeue capture buffer */
	ret = ioctl(pvdev->fd, VIDIOC_DQBUF, &buf);
	if (ret < 0) {
		LOG("Cap VIDIOC_DQBUF");
		return ret;
	}
#ifdef LOGTIME
	gettimeofday( &ctime, NULL );
	LOG("DQ cap: %ld(%ld)\n", 
		(ctime.tv_sec-btime.tv_sec)*1000000 + ctime.tv_usec-btime.tv_usec, 
		(ctime.tv_sec-ltime.tv_sec)*1000000 + ctime.tv_usec-ltime.tv_usec);
	ltime = ctime;
#endif

	//print_status(pvdev->fd);

	if (frame_count % frame_devisor == 0)
		update_texture((void *)pvdev->buff_info[buf.index].start, pvdev->cap_width, pvdev->cap_height);

#ifdef LOGTIME
	gettimeofday( &ctime, NULL );
	LOG("Process done: %ld(%ld)\n", 
		(ctime.tv_sec-btime.tv_sec)*1000000 + ctime.tv_usec-btime.tv_usec, 
		(ctime.tv_sec-ltime.tv_sec)*1000000 + ctime.tv_usec-ltime.tv_usec);
	ltime = ctime;
#endif

	pthread_testcancel();
	ret = ioctl(pvdev->fd, VIDIOC_QBUF, &buf);
	if (ret < 0) {
		LOG("Cap VIDIOC_QBUF");
		return ret;
	}
#ifdef LOGTIME
	gettimeofday( &ctime, NULL );
	LOG("Q cap: %ld(%ld)\n", 
		(ctime.tv_sec-btime.tv_sec)*1000000 + ctime.tv_usec-btime.tv_usec, 
		(ctime.tv_sec-ltime.tv_sec)*1000000 + ctime.tv_usec-ltime.tv_usec);
	ltime = ctime;
#endif

	return 0;
}

static int video_stop_cap(struct video_dev *pvdev)
{
	int a, ret;
	
	LOG("\n%s: Stream off!!\n", CAPTURE_NAME);

	a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(pvdev->fd, VIDIOC_STREAMOFF, &a);
	if (ret < 0) {
		LOG("VIDIOC_STREAMOFF");
		return ret;
	}

	return 0;
}

struct video_thread{
	pthread_t th_video;
	int video_running;
	pthread_mutex_t mutex;
};

static struct video_thread video_thd;

void handle_input(void);

static void* Show_video(void* v)
{
	if (initCapture(CAPTURE_DEVICE,&videodev) < 0) {
		DPRINTF("Failed to open:%s\n", CAPTURE_DEVICE);
		return NULL;
	}

	if (display_init(fbinfo.fb_width, fbinfo.fb_height)) {
		DPRINTF("Failed to init display system\n");
		return NULL;
	}

	//start capture
	if( video_start_cap(&videodev)<0){
		return NULL;
	}

	while(1){
		pthread_mutex_lock(&video_thd.mutex);
		pthread_mutex_unlock(&video_thd.mutex);

		if(video_cap_frame(&videodev)<0)
			break;

		if (frame_count % frame_devisor == 0)
			render_frame();

		//handle_input();

		frame_count++;
	}

	display_exit();
	video_stop_cap(&videodev);
	return NULL;
}

int video_pause(void)
{
	pthread_mutex_lock(&video_thd.mutex);
	return 0;
}

int video_run(void)
{
	pthread_mutex_unlock(&video_thd.mutex);
	return 0;
}

int video_start(void)
{
	pthread_mutexattr_t attr;

	if(video_thd.video_running){
		video_stop();
	}

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_ERRORCHECK_NP);
	pthread_mutex_init(&video_thd.mutex, NULL);

	pthread_create(&video_thd.th_video, NULL, (void*(*)(void*))Show_video, (void*)NULL);
	video_thd.video_running=1;

	return 0;
}

int video_stop(void)
{
	if(video_thd.video_running){
		video_run();
		pthread_cancel(video_thd.th_video);
		pthread_join(video_thd.th_video, NULL);
		video_thd.video_running = 0;
	}

	video_close(&videodev);
	pthread_mutex_destroy(&video_thd.mutex);

	return 0;
}

int video_init(FILE *log)
{
	logfile = log;
	video_thd.video_running = 0;
	if(Getfb_info(DEFAULT_FB_DEVICE)<0)
		return -1;

	return 0;
}

