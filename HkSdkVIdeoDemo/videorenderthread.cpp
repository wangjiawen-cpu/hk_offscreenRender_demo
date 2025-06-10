#include "videorenderthread.h"


VideoRenderThread::VideoRenderThread(QOpenGLContext* sharedContext, QObject* parent)
    : QThread(parent), m_mainContext(sharedContext)
{
    // 回调转发到队列
    m_yv12DataCallback = [this](int nPort,char * pBuf,int nSize, FRAME_INFO* info, void*, int) {
        QByteArray data(pBuf, info->nWidth * info->nHeight * 3 / 2);
        queueFrame(data, info->nWidth, info->nHeight);
    };
}

VideoRenderThread::~VideoRenderThread()
{
    m_running = false;

    if (isRunning())
        wait();

    delete m_context;   // 删除上下文
    delete m_surface;   // 删除离屏表面
}

void VideoRenderThread::queueFrame(const QByteArray &yv12Data, int width, int height)
{
    //qDebug() << width << height;

    QMutexLocker lock(&m_queueMutex);
    if (m_frameQueue.size() > 5) // 控制队列最大长度
    {
        m_frameQueue.dequeue();  // 移除最旧帧
    }
    m_frameQueue.enqueue(qMakePair(yv12Data, QSize(width, height)));// 添加新帧
}

void VideoRenderThread::setFps(const uint num)
{
    m_targetFPS = num;
    m_frameInterval = 1000.0f / m_targetFPS;
}

void VideoRenderThread::stopThread()
{
    m_running = false;
    this->wait();
}

void VideoRenderThread::run()
{
    init();

    // 绑定上下文到当前线程
    if (!m_context->makeCurrent(m_surface)) {
        qWarning() << "MakeCurrent failed in render thread";
        return;
    }

    // 初始化OpenGL函数
    initializeOpenGLFunctions();

    m_running = true;
    m_frameTimer.start(); // 启动全局计时器

//    int nrAttributes = 0;
//    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
//    qDebug() << "opengl 最大支持顶点属性上限 ： " << nrAttributes;

    while (m_running)
    {
        m_context->makeCurrent(m_surface);
        const qint64 frameStart = QDateTime::currentMSecsSinceEpoch(); // 精确到毫秒

        // 处理队列中的帧数据
        QPair<QByteArray, QSize> frame;
        {
            QMutexLocker lock(&m_queueMutex);
            if (!m_frameQueue.isEmpty())
            {
                frame = m_frameQueue.dequeue(); // 取出最早一帧
            }
        }

        if (!frame.first.isEmpty()) { // 有效帧处理 将纹理传入gpu
            updateYUVFrame(reinterpret_cast<const uint8_t*>(frame.first.constData()),
                           frame.second.width(),
                           frame.second.height());

            const YUVBuffer& frontBuffer = m_buffers[m_frontIndex.loadRelaxed()];

            emit frameReady(frontBuffer);
            //            // 调试保存原始帧
            //            yv12ToQImage(reinterpret_cast<const uint8_t*>(frame.first.constData()),
            //                         frame.second.width(), frame.second.height()).save("/home/ubuntu/frame.png");

        }

        // 清理待删除纹理
        if (!m_pendingDelete.isEmpty()) {
            glDeleteTextures(m_pendingDelete.size(), m_pendingDelete.constData());
            m_pendingDelete.clear();
        }
        m_context->doneCurrent();
        // 计算实际耗时
        const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - frameStart;

        QThread::msleep(m_frameInterval - elapsed); //动态调整延时控制fps
    }

    // 资源清理
    m_textureMutex.lock();
    for (auto& buffer : m_buffers) {            // 删除所有纹理
        destroyVideoTexture(buffer);
    }
    m_textureMutex.unlock();
    m_context->doneCurrent();  // 释放上下文
    qDebug() << "opengl thread exit!";
}

void VideoRenderThread::init()
{
    // 创建线程专属的OpenGL上下文
    m_context = new QOpenGLContext;
    m_context->setFormat(m_mainContext->format());
    m_context->setShareContext(m_mainContext); // 关键设置：显式共享
    if(!m_context->create()) {
        qCritical() << "Context create failed. Main ctx:"
                    << m_mainContext->format()
                    << "Thread ctx:" << m_context->format();
        return;
    }

    // 添加共享验证
    if (!m_context->shareContext()) {
        qCritical() << "Context sharing failed!";
        return;
    }

    // 创建线程专属的离屏表面
    m_surface = new QOffscreenSurface;
    m_surface->setFormat(m_context->format());
    m_surface->create();

}

void VideoRenderThread::updateYUVFrame(const uint8_t *yv12Data, int width, int height)
{
    QMutexLocker locker(&m_textureMutex);

    // 获取后台缓冲索引（1 - 前台索引）
    const int backIndex = 1 - m_frontIndex.loadRelaxed();
    YUVBuffer& backBuffer = m_buffers[backIndex];

    // 创建或更新纹理
    auto updateTexture = [&](GLTexture* tex, int w, int h, const void* data) {
        if(w <= 0 || h <= 0 || w % 2 != 0 || h % 2 != 0) {
            qWarning() << "Invalid texture size:" << w << "x" << h;
            return;
        }
        Q_ASSERT(QOpenGLContext::currentContext() == m_context);
        if(tex->width != w || tex->height != h) {
            //if(tex->id) glDeleteTextures(1, &tex->id);
            if(tex->id) m_pendingDelete.append(tex->id);
            createVideoTexture(tex, w, h);
        }
        glBindTexture(GL_TEXTURE_2D, tex->id);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        //  上传数据
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, data);
    };

    // Y分量
    updateTexture(&backBuffer.y, width, height, yv12Data);

    // V分量
    const uint8_t* vData = yv12Data + width * height;
    updateTexture(&backBuffer.v, width/2, height/2, vData);

    // u分量
    const uint8_t* uData = vData + (width/2) * (height/2);
    updateTexture(&backBuffer.u, width/2, height/2, uData);

    m_frontIndex.storeRelaxed(backIndex);
    // 添加内存屏障确保数据同步
    std::atomic_thread_fence(std::memory_order_release);
}

void VideoRenderThread::initYV12Data(const uint8_t *data, int w, int h)
{
    if(w <= 0 || h <= 0 || w % 2 != 0 || h % 2 != 0) {
        qWarning() << "Invalid texture size:" << w << "x" << h;
        return;
    }
    m_context->makeCurrent(m_surface);

    YUVBuffer& buffer = m_buffers[m_frontIndex];

    // Y分量
    glBindTexture(GL_TEXTURE_2D, buffer.y.id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
                    GL_RED, GL_UNSIGNED_BYTE, data);

    // V分量
    const uint8_t* vData = data + w * h;
    glBindTexture(GL_TEXTURE_2D, buffer.v.id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w/2, h/2,
                    GL_RED, GL_UNSIGNED_BYTE, vData);

    // U分量
    const uint8_t* uData = vData + (w/2) * (h/2);
    glBindTexture(GL_TEXTURE_2D, buffer.u.id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w/2, h/2,
                    GL_RED, GL_UNSIGNED_BYTE, uData);

    m_context->doneCurrent();
}

void VideoRenderThread::yv12DataToGPUCallBack(int nPort, char *pBuf, int nSize, FRAME_INFO *pFrameInfo, void *nUser, int nReserved2)
{
    //qDebug()<<"nSize:"<<nSize<<pFrameInfo->nWidth<<pFrameInfo->nHeight<<pFrameInfo->nType<<pFrameInfo->nFrameRate;

    updateYUVFrame((const uint8_t *)pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight);

    //    // 通过元调用系统保证线程安全（回到主线程）
    //    QMetaObject::invokeMethod(this, [this, pBuf, pFrameInfo]() {
    //        updateYUVFrame((const uint8_t *)pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight);
    //    }, Qt::QueuedConnection);
}


GLuint VideoRenderThread::createShaderProgram(const QString& vertexPath, const QString& FragmentPath)
{
    // 加载顶点着色器
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexPath.toStdString().c_str());
    if(vertexShader == 0)
    {
        qDebug() << "createShaderProgram loadShader GL_VERTEX_SHADER error!";
        return 0;
    }

    // 加载片段着色器
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, FragmentPath.toStdString().c_str());
    if(fragmentShader == 0) {
        glDeleteShader(vertexShader);
        qDebug() << "createShaderProgram loadShader GL_FRAGMENT_SHADER error!";
        return 0;
    }

    // 创建着色器程序
    GLuint shader = glCreateProgram();
    // 链接着色器程序
    glAttachShader(shader, vertexShader);
    glAttachShader(shader, fragmentShader);
    glLinkProgram(shader);

    // 检查链接状态
    GLint success;
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetProgramInfoLog(shader, 512, nullptr, infoLog);
        qWarning() << "Shader program linking failed:\n" << infoLog;
    }

    // 清理着色器对象
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shader;
}

GLuint VideoRenderThread::loadShader(GLenum type, const QString &path)
{
    // 读取着色器文件内容
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open shader file:" << path
                   << "Error:" << file.errorString();
        return 0;
    }
    QByteArray source = file.readAll();
    const GLchar* src = source.constData();
    // 创建并编译着色器
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    // 检查编译状态
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success || shader==0) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        qWarning() << (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment")
                   << "shader compilation failed (" << path << "):\n" << infoLog;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

QImage VideoRenderThread::yv12ToQImage(const uint8_t *yv12Data, int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    const uint8_t* yPlane = yv12Data;
    const uint8_t* vPlane = yv12Data + width * height;
    const uint8_t* uPlane = vPlane + (width/2) * (height/2);

    for (int y = 0; y < height; ++y) {
        QRgb* scanline = reinterpret_cast<QRgb*>(image.scanLine(y));

        for (int x = 0; x < width; ++x) {
            // 获取Y分量
            const uint8_t Y = yPlane[y * width + x];

            // 计算UV坐标（注意防止越界）
            const int uvX = std::min(x / 2, (width/2) - 1);
            const int uvY = std::min(y / 2, (height/2) - 1);

            // 获取UV分量
            const uint8_t V = vPlane[uvY * (width/2) + uvX];
            const uint8_t U = uPlane[uvY * (width/2) + uvX];

            // YUV转RGB（使用BT.601系数）
            int r = Y + 1.402 * (V - 128);
            int g = Y - 0.344136 * (U - 128) - 0.714136 * (V - 128);
            int b = Y + 1.772 * (U - 128);

            // 钳制到[0,255]范围
            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            scanline[x] = qRgb(r, g, b);
        }
    }

    return image;
}

QVector<uint8_t> VideoRenderThread::rgbToYV12(QColor rgb, int width, int height)
{
    // 验证参数有效性
    Q_ASSERT(width > 0 && height > 0);
    Q_ASSERT(width % 2 == 0 && height % 2 == 0);

    // 准备数据缓冲区
    const int ySize = width * height;
    const int uvSize = (width/2) * (height/2);
    QVector<uint8_t> yv12Data(ySize + 2 * uvSize);

    // 获取归一化RGB值（范围0.0-1.0）
    float r = rgb.redF();
    float g = rgb.greenF();
    float b = rgb.blueF();

    // BT.601转换系数
    float yCoeff  = 16.0f + 219.0f * (0.299f*r + 0.587f*g + 0.114f*b);
    float uCoeff  = 128.0f + 224.0f * (-0.14713f*r -0.28886f*g +0.436f*b);
    float vCoeff  = 128.0f + 224.0f * (0.615f*r -0.51499f*g -0.10001f*b);

    // 计算并限制范围
    const uint8_t yVal = qBound(16, static_cast<int>(yCoeff + 0.5f), 235);
    const uint8_t uVal = qBound(16, static_cast<int>(uCoeff + 0.5f), 240);
    const uint8_t vVal = qBound(16, static_cast<int>(vCoeff + 0.5f), 240);

    // 填充Y平面
    std::fill_n(yv12Data.data(), ySize, yVal);

    // 填充V平面（YV12中V在前）
    uint8_t* vPlane = yv12Data.data() + ySize;
    std::fill_n(vPlane, uvSize, vVal);

    // 填充U平面
    uint8_t* uPlane = vPlane + uvSize;
    std::fill_n(uPlane, uvSize, uVal);

    return yv12Data;
}

void VideoRenderThread::createVideoTexture(GLTexture* tex, int w, int h)
{
    if(w <= 0 || h <= 0 || w % 2 != 0 || h % 2 != 0) {
        qWarning() << "Invalid texture size:" << w << "x" << h;
        return;
    }
    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 分配存储空间
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    tex->width = w;
    tex->height = h;
}

void VideoRenderThread::destroyVideoTexture(YUVBuffer &buffer)
{
    if(buffer.y.id) glDeleteTextures(1, &buffer.y.id);
    if(buffer.u.id) glDeleteTextures(1, &buffer.u.id);
    if(buffer.v.id) glDeleteTextures(1, &buffer.v.id);
    buffer = YUVBuffer{};
}
