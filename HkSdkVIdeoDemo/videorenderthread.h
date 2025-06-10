#ifndef VIDEORENDERTHREAD_H
#define VIDEORENDERTHREAD_H


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
#include <QFile>
#include <QThread>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QQueue>
#include <QElapsedTimer>
#include <QDateTime>
#include "hikvisionipccamera.h"

struct GLTexture {
    GLuint id = 0;
    int width = 0;
    int height = 0;
};
Q_DECLARE_METATYPE(GLTexture) // 声明元类型

struct YUVBuffer {
    GLTexture y;
    GLTexture u;
    GLTexture v;
};
Q_DECLARE_METATYPE(YUVBuffer) // 声明元类型

class VideoRenderThread : public QThread, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit VideoRenderThread(QOpenGLContext* sharedContext, QObject* parent = nullptr);
    ~VideoRenderThread();

    YUV_ImageCallBack m_yv12DataCallback;

    void queueFrame(const QByteArray& yv12Data, int width, int height);// 帧数据入队
    void setFps(const uint num);

    void stopThread();

signals:
    void frameReady(YUVBuffer);

protected:
    void run() override;


private:
    // 双缓冲相关成员
    YUVBuffer m_buffers[2];      // 双纹理缓冲
    QAtomicInt m_frontIndex = 0;      // 当前显示纹理索引
    QMutex m_textureMutex;              // 纹理操作互斥锁
    QVector<GLuint> m_pendingDelete; // 延迟删除的纹理

    uint m_targetFPS = 25; // 默认帧率
    float m_frameInterval = 40;      // 精确间隔（1000/25）
    QElapsedTimer m_frameTimer;           // 帧计时器
    float m_lastElapsed = 0;              // 上一帧耗时

    const float vertices[20] = {
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };

    // 线程控制相关
    QOpenGLContext* m_context;   // 线程上下文
    QOpenGLContext* m_mainContext;   // 共享上下文
    QOffscreenSurface* m_surface;     // 离屏渲染表面
    QMutex m_queueMutex;
    QQueue<QPair<QByteArray, QSize>> m_frameQueue;  // 帧数据队列
    std::atomic_bool m_running{false};

private:
    //视频渲染
    void init();  //视频渲染初始化
    void createVideoTexture(GLTexture* tex, int w, int h);  //创建视频纹理
    void destroyVideoTexture(YUVBuffer& buffer);            //销毁视频纹理
    void yv12DataToGPUCallBack(int nPort,char * pBuf,int nSize,FRAME_INFO * pFrameInfo, void* nUser,int nReserved2);
    void updateYUVFrame(const uint8_t* yv12Data, int width, int height);   // 更新YV12视频帧（延迟删除）
    void initYV12Data(const uint8_t* data, int w, int h);   //初始化显示颜色

    //工具函数
    GLuint createShaderProgram(const QString& vertexPath, const QString& FragmentPath);                               //创建着色器
    GLuint loadShader(GLenum type, const QString& path);
    QImage yv12ToQImage(const uint8_t* yv12Data, int width, int height);
    QVector<uint8_t> rgbToYV12(QColor rgb, int width, int height);
};
#endif // VideoRenderThreadEX_H
