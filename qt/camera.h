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

    static const int CAPTURE_MAX_BUFFER = 5;
    static char *CAPTURE_DEVICE;

    struct buf_info{
        int index;
        unsigned int length;
        void *start;
    };

    struct video_dev
    {
        int fd;
        int cap_width, cap_height;
        struct buf_info buff_info[CAPTURE_MAX_BUFFER];
        int numbuffer;
    } videodev;

    void vidioc_enuminput(int fd);

private:
    CameraTexture *m_texture;
    unsigned int frame_count;
    int frame_devisor;
    bool m_running;
    QMutex m_wait_mutex;
    QWaitCondition m_wait;
    QSGGeometryNode *m_node;

    int initCapture();
    int startCapture();
    int captureFrame();
    int stopCapture();
    void closeCapture();

    void run();

    void updateTexture(const uchar *data, int width, int height);
    virtual void textureProcess(const uchar *data, int width, int height);
    virtual int subInitCapture() { return 0; }
};

#endif // CAMERA_H
