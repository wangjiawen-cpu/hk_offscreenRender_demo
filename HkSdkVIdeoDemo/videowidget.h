#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <videorenderthread.h>

class VideoWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
public:
    explicit VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget();

    // 初始化视频流
    void startVideoStream();
    YUV_ImageCallBack& getCallbackPointer();

protected:
    void initializeGL() override;
    void paintGL() override;
    //void resizeGL(int w, int h) override;

private slots:
    // 接收渲染线程的纹理更新
    void onFrameReady(YUVBuffer);


private:
    //视频渲染
    GLuint m_videoRenderShader = 0;
    GLuint m_videoVao = 0;
    GLuint m_videoVbo = 0;

    QAtomicInt m_texY = 0;  // Y分量纹理ID（原子操作保证线程安全）
    QAtomicInt m_texU = 0;  // U分量纹理ID
    QAtomicInt m_texV = 0;  // V分量纹理ID
    QSize m_frameSize;      // 视频帧尺寸
    QMutex m_queueMutex;
    VideoRenderThread* m_thread; // 关联的渲染线程

    const float vertices[20] = {
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };


private:
    //视频渲染
    void VideoRenderInit();  //视频渲染初始化

    //工具函数
    GLuint createShaderProgram(const QString& vertexPath, const QString& FragmentPath);                               //创建着色器
    GLuint loadShader(GLenum type, const QString& path);
    QImage yv12ToQImage(const uint8_t* yv12Data, int width, int height);
    QVector<uint8_t> rgbToYV12(QColor rgb, int width, int height);
};

#endif // VIDEOWIDGET_H
