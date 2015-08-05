#include "camera.h"
#include "imagestream.h"
#include "camera_texture.h"
#include "camera_texture.h"
#include "yuv2rgb_material.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <QDebug>
#include <QSGGeometryNode>

Camera::Camera(QObject *parent)
    : QThread(parent), m_image(0), m_texture(0),
      frame_count(0), frame_devisor(1), m_running(false),
      m_node(0)
{
    videodev.fd = -1;
}

Camera::~Camera()
{
    closeCapture();

    if (m_texture)
        delete m_texture;
    if (m_image)
        delete m_image;
}

QSGGeometryNode *Camera::createNode()
{
    QSGGeometry *geometry;

    if (!m_texture)
        m_texture = new CameraTexture(m_image->getWidth(), m_image->getHeight());

    if (!m_node) {
        m_node = new QSGGeometryNode;
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);
        geometry->setDrawingMode(GL_TRIANGLE_STRIP);
        m_node->setGeometry(geometry);
        m_node->setFlag(QSGNode::OwnsGeometry);

        YUV2RGBMaterial *material = new YUV2RGBMaterial;
        material->setTexture(m_texture);
        m_node->setMaterial(material);
        m_node->setFlag(QSGNode::OwnsMaterial);
    }

    return m_node;
}

void Camera::updateGeometry(qreal x, qreal y, qreal width, qreal height)
{
    QSGGeometry::TexturedPoint2D *vertices = m_node->geometry()->vertexDataAsTexturedPoint2D();

    vertices[0].x = x;
    vertices[0].y = y + height;
    vertices[0].tx = 0;
    vertices[0].ty = 1;

    vertices[1].x = x;
    vertices[1].y = y;
    vertices[1].tx = 0;
    vertices[1].ty = 0;

    vertices[2].x = x + width;
    vertices[2].y = y + height;
    vertices[2].tx = 1;
    vertices[2].ty = 1;

    vertices[3].x = x + width;
    vertices[3].y = y;
    vertices[3].tx = 1;
    vertices[3].ty = 0;
}

void Camera::startStream()
{
    m_running = true;
    m_wait.wakeAll();
}

void Camera::stopStream()
{
    m_running = false;
}

void Camera::run()
{
    if (initCapture() < 0)
        return;

    if (startCapture() < 0)
        return;

    while (1) {
        if (captureFrame() < 0)
            break;
    }

    stopCapture();
    closeCapture();
}

void Camera::textureProcess(const uchar *data, int width, int height)
{
    Q_UNUSED(data);
    Q_UNUSED(width);
    Q_UNUSED(height);
}

void Camera::updateTexture(const uchar *data, int width, int height)
{
    if (!m_running)
        return;

    textureProcess(data, width, height);
    m_image->swapImage();
    emit imageChanged();
}

void Camera::updateMaterial()
{
    m_texture->updateFrame(m_image->getFrontImage());
}

int Camera::startCapture()
{
    int a, ret;

    /* Start Streaming. on capture device */
    a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(videodev.fd, VIDIOC_STREAMON, &a);
    if (ret < 0) {
        qDebug() << "capture VIDIOC_STREAMON error fd=" << videodev.fd;
        return ret;
    }
    qDebug() << "Stream on...";

    return 0;
}

int Camera::captureFrame()
{
    int ret;
    struct v4l2_buffer buf;

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;

    /* Dequeue capture buffer */
    ret = ioctl(videodev.fd, VIDIOC_DQBUF, &buf);
    if (ret < 0) {
        qDebug() << "Cap VIDIOC_DQBUF";
        return ret;
    }

    if (frame_count++ % frame_devisor == 0)
        updateTexture((uchar *)videodev.buff_info[buf.index].start, videodev.cap_width, videodev.cap_height);

    ret = ioctl(videodev.fd, VIDIOC_QBUF, &buf);
    if (ret < 0) {
        qDebug() << "Cap VIDIOC_QBUF";
        return ret;
    }

    return 0;
}

int Camera::stopCapture()
{
    int a, ret;

    qDebug() << "Stream off!!\n";

    a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(videodev.fd, VIDIOC_STREAMOFF, &a);
    if (ret < 0) {
        qDebug() << "VIDIOC_STREAMOFF";
        return ret;
    }

    return 0;
}

void Camera::closeCapture()
{
    int i;
    struct buf_info *buff_info;

    /* Un-map the buffers */
    for (i = 0; i < CAPTURE_MAX_BUFFER; i++){
        buff_info = &videodev.buff_info[i];
        if (buff_info->start) {
            munmap(buff_info->start, buff_info->length);
            buff_info->start = NULL;
        }
    }

    if (videodev.fd >= 0) {
        close(videodev.fd);
        videodev.fd = -1;
    }
}
