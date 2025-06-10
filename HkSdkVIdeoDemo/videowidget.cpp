#include "videowidget.h"

//逻辑框架
//[主线程] VideoWidget                 [渲染线程] VideoRenderThread
//|                                     |
//|--- initializeGL()                   |--- run()
//|   |                                    |--- 创建共享上下文
//|   |--- 创建着色器/VAO/VBO                |--- 接收YUV数据
//|   |--- 启动渲染线程                      |--- 更新三缓冲纹理
//|                                     |--- 发送frameReady信号
//|
//|--- paintGL()
//|   |--- 绑定接收的纹理
//|   |--- 执行渲染
//|
//|--- onFrameReady()
//    |--- 更新原子变量存储纹理ID



VideoWidget::VideoWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

VideoWidget::~VideoWidget()
{
    glDeleteVertexArrays(1, &m_videoVao);      // 删除VAO
    glDeleteBuffers(1, &m_videoVbo);           // 删除VBO
    glDeleteProgram(m_videoRenderShader);       // 删除着色器程序

    m_thread->stopThread();
    delete m_thread;
}

void VideoWidget::startVideoStream()
{
    connect(m_thread, &VideoRenderThread::frameReady,
            this, &VideoWidget::onFrameReady,
            Qt::QueuedConnection); // 明确指定队列连接
    m_thread->start();
}

YUV_ImageCallBack& VideoWidget::getCallbackPointer()
{
    return m_thread->m_yv12DataCallback;
}

void VideoWidget::initializeGL() {

    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    VideoRenderInit();

    // 创建专属渲染线程
    m_thread = new VideoRenderThread(context(), this);

    startVideoStream();
}

void VideoWidget::paintGL() {
    makeCurrent();
    Q_ASSERT(QOpenGLContext::currentContext() == context());

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        qWarning() << "OpenGL error before draw:" << err;
    }
    if(m_texY.loadRelaxed() == 0) return;

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_videoRenderShader);
    glBindVertexArray(m_videoVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_videoVbo);

    // 绑定共享纹理
    m_queueMutex.lock(); // 纹理操作加锁
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texY.loadRelaxed());
    GLint yTexLoc = glGetUniformLocation(m_videoRenderShader, "yTex");
    if (yTexLoc == -1) qWarning() << "yTex uniform not found!";
    glUniform1i(yTexLoc, 0); // 显式指定纹理单元

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_texU.loadRelaxed());
    GLint uTexLoc = glGetUniformLocation(m_videoRenderShader, "uTex");
    if (yTexLoc == -1) qWarning() << "yTex uniform not found!";
    glUniform1i(uTexLoc, 1); // 显式指定纹理单元

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_texV.loadRelaxed());
    GLint vTexLoc = glGetUniformLocation(m_videoRenderShader, "vTex");
    if (yTexLoc == -1) qWarning() << "yTex uniform not found!";
    glUniform1i(vTexLoc, 2); // 显式指定纹理单元

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    m_queueMutex.unlock();           // 解锁纹理
    doneCurrent();
}

void VideoWidget::onFrameReady(YUVBuffer yuv) {
    QMutexLocker locker(&m_queueMutex);
    // 添加内存屏障确保数据可见
    std::atomic_thread_fence(std::memory_order_acquire);

    qDebug() << "Received textures - Y:" << yuv.y.id
             << "U:" << yuv.u.id
             << "V:" << yuv.v.id;
    m_texY.storeRelaxed(yuv.y.id);
    m_texU.storeRelaxed(yuv.u.id);
    m_texV.storeRelaxed(yuv.v.id);
    m_frameSize = QSize(yuv.y.width, yuv.y.height);

    update();
}

void VideoWidget::VideoRenderInit()
{
    // 创建着色器程序
    m_videoRenderShader = createShaderProgram(":/shaders/yuv2rgb.vert", ":/shaders/yuv2rgb.frag");
    if(m_videoRenderShader == 0)
    {
        qDebug() << "video render shader programe create error!";
    }

    glGenVertexArrays(1, &m_videoVao);
    glGenBuffers(1, &m_videoVbo);

    glBindVertexArray(m_videoVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_videoVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

GLuint VideoWidget::createShaderProgram(const QString &vertexPath, const QString &FragmentPath)
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

GLuint VideoWidget::loadShader(GLenum type, const QString &path)
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

QImage VideoWidget::yv12ToQImage(const uint8_t *yv12Data, int width, int height)
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

QVector<uint8_t> VideoWidget::rgbToYV12(QColor rgb, int width, int height)
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

