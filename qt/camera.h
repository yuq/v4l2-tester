#ifndef CAMERA_H
#define CAMERA_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include <linux/videodev2.h>

class ImageStream;
class CameraTexture;
class QSGGeometryNode;

class Camera : public QThread
{
    Q_OBJECT
public:
    Camera(QObject *parent = 0);
    virtual ~Camera();

    void startStream();
    void stopStream();

    QSGGeometryNode *createNode();
    void updateGeometry(qreal x, qreal y, qreal width, qreal height);
    void updateMaterial();

signals:
    void imageChanged();

protected:
    ImageStream *m_image;
    CameraTexture *m_texture;
    unsigned int frame_count;
    int frame_devisor;

    static const int CAPTURE_MAX_BUFFER = 5;

    struct buf_info{
        int index;
        unsigned int length;
        void *start;
    };

    struct video_dev
    {
        int fd;
        unsigned int offset[2];
        int cap_width, cap_height;

        struct v4l2_buffer capture_buf;
        struct buf_info buff_info[CAPTURE_MAX_BUFFER];
        int numbuffer;
    } videodev;

    void closeCapture();
    void updateTexture(const uchar *data, int width, int height);

private:
    bool m_running;
    QMutex m_wait_mutex;
    QWaitCondition m_wait;
    QSGGeometryNode *m_node;

    virtual int initCapture() { return 0; }
    int startCapture();
    int captureFrame();
    int stopCapture();

    void run();

    virtual void textureProcess(const uchar *data, int width, int height);
};

#endif // CAMERA_H
