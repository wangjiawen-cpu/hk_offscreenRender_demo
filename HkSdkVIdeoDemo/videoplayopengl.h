#ifndef VIDEOPLAYOPENGL_H
#define VIDEOPLAYOPENGL_H

#include <QWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <QDebug>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QTimer>
#include <QOpenGLTexture>
#include <QImageReader>
#include <QMutex>
#include "hikvisionipccamera.h"

class videoPlayOpengl : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit videoPlayOpengl(QWidget *parent = nullptr);
    ~videoPlayOpengl();

    void updateVideo(const QImage &frame);

    YUV_ImageCallBack m_yv12ToImageCallback;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;



private:
    QOpenGLShaderProgram m_program;
    QOpenGLVertexArrayObject m_VAO;
    QOpenGLBuffer m_VBO;

    // 双缓冲相关成员
    QOpenGLTexture* m_textures[2];      // 双纹理缓冲
    QAtomicInt m_frontIndex = 0;      // 当前显示纹理索引
    QMutex m_textureMutex;              // 纹理操作互斥锁
    QSize m_videoSize;                  // 视频帧尺寸
    QImage::Format m_currentFormat;

    const float vertices[20] = {
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };

    void createTexture(int index, const QSize &size);
    void swapBuffers();
    QImage yv12ToQImage(const uint8_t* yv12Data, int width, int height);
    void yv12ToImageCallBack(int nPort,char * pBuf,int nSize,FRAME_INFO * pFrameInfo, void* nUser,int nReserved2);

};

#endif // VIDEOPLAYOPENGL_H

