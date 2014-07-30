#ifndef SUNXI_TVD_CAMERA_H
#define SUNXI_TVD_CAMERA_H

#include <QThread>
#include <QWaitCondition>
#include "imagestream.h"
#include <linux/videodev2.h>

class SunxiTVDCamera : public QThread
{
	Q_OBJECT
public:
	SunxiTVDCamera(ImageStream *ims, QObject *parent = 0);
	~SunxiTVDCamera();

	void startStream();
	void stopStream();

private:
	bool running;
	QMutex m_wait_mutex;
	QWaitCondition m_wait;
	ImageStream *m_image;

	unsigned int frame_count;
	int frame_devisor;

	static const int CAPTURE_MAX_BUFFER = 5;

	/* structure used to store information of the buffers */
	struct buf_info{
		int index;
		unsigned int length;
		void *start;
	};

	struct video_dev
	{
		int fd;	//for video device
		unsigned int offset[2];	//capture data C offset
		int cap_width, cap_height;

		struct v4l2_buffer capture_buf;
		struct buf_info buff_info[CAPTURE_MAX_BUFFER];
		int numbuffer;
	} videodev;

	int initCapture();
	void closeCapture();
	int startCapture();
	int captureFrame();
	int stopCapture();

	void run();

	void updateTexture(const uchar *data, int width, int height);

signals:
	void imageChanged();
};

#endif

