#include "videoplayopengl.h"


videoPlayOpengl::videoPlayOpengl(QWidget *parent)
{
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);

    m_textures[0] = m_textures[1] = nullptr;

    //c++新语法，可能存在线程安全
    //    m_callback = std::bind(
    //                &videoPlayOpengl::yv12ToImageCallBack,
    //                this,
    //                std::placeholders::_1,
    //                std::placeholders::_2,
    //                std::placeholders::_3,
    //                std::placeholders::_4,
    //                std::placeholders::_5,
    //                std::placeholders::_6
    //                );

    //表达式就是回调
    m_yv12ToImageCallback = [this](int port, char* buf, int size,
            FRAME_INFO* info, void* user, int res)
    {
        yv12ToImageCallBack(port, buf, size, info, user, res);
    };
}

videoPlayOpengl::~videoPlayOpengl()
{
    makeCurrent();
    for(int i = 0; i < 2; ++i) {
        if(m_textures[i]) {
            m_textures[i]->destroy();
            delete m_textures[i];
        }
    }
    m_VAO.destroy();
    m_VBO.destroy();
    doneCurrent();
}



void videoPlayOpengl::initializeGL()
{
    initializeOpenGLFunctions();  //必须先初始化将所用函数正真指向显卡函，要不然只是本地的函数指针，无法调用显卡

    glEnable(GL_MULTISAMPLE);//来启用多重采样。

    // 设置视口大小
    glViewport(0, 0, width(), height());


    //加载着色器
    if(!m_program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vertex.shader"))
    {
        qDebug() << "QOpenGLShader::Vertex ERROR! >> " << m_program.log();
    }

    if(!m_program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fragment.shader"))
    {
        qDebug() << "QOpenGLShader::Fragment ERROR! >> " << m_program.log();
    }

    if(!m_program.link())
    {
        qDebug() << "shaderProgram.link ERROR! >> " << m_program.log();
    }

    if(false == m_program.bind())
    {
        qDebug() << "initializeGL() : 着色器绑定失败！" << m_program.log();
    }
    else
    {
        qDebug() << "initializeGL() : 着色器绑定成功!";
    }

    //创建VAO
    m_VAO.create();

    //创建VBO
    m_VBO.create();
    m_VBO.setUsagePattern(QOpenGLBuffer::StaticDraw);

    m_VAO.bind();
    m_VBO.bind();
    m_VBO.allocate(vertices, sizeof(vertices)); //VBO绑定内存
    m_program.setAttributeBuffer("aPos", GL_FLOAT, 0, 3, 5 * sizeof(float));
    m_program.enableAttributeArray("aPos");
    m_program.setAttributeBuffer("aTexCord", GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));
    m_program.enableAttributeArray("aTexCord");
    m_VBO.release();

    m_program.release();
    m_VAO.release();

    // 初始化双缓冲纹理
    for(int i = 0; i < 2; ++i) {
        m_textures[i] = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_textures[i]->setFormat(QOpenGLTexture::RGBA8_UNorm);
        m_textures[i]->setSize(m_videoSize.width(), m_videoSize.height());
        m_textures[i]->setMipLevels(0);
        m_textures[i]->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        m_textures[i]->create();
        m_textures[i]->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
    }

}

void videoPlayOpengl::updateVideo(const QImage &frame)
{
    if(frame.isNull() || !m_textures[0]) return;

    QImage glFrame = frame.convertToFormat(QImage::Format_RGBA8888);
    glFrame = glFrame.mirrored(); // 同时处理镜像

    {
        QMutexLocker locker(&m_textureMutex);
        makeCurrent();

        bool needRebuild = (m_videoSize != glFrame.size()) ||
                (m_currentFormat != glFrame.format()) ||
                (m_textures[0]->format() != QOpenGLTexture::RGBA8_UNorm);
        // 动态调整纹理尺寸
        if(needRebuild) {
            m_videoSize = frame.size();
            m_currentFormat = glFrame.format();

            for(int i = 0; i < 2; ++i) {
                m_textures[i]->destroy();
                delete m_textures[i];

                m_textures[i] = new QOpenGLTexture(QOpenGLTexture::Target2D);
                m_textures[i]->setFormat(QOpenGLTexture::RGBA8_UNorm);
                m_textures[i]->setSize(m_videoSize.width(), m_videoSize.height());
                m_textures[i]->setMipLevels(0);
                m_textures[i]->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
                m_textures[i]->create();
                m_textures[i]->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
            }

        }
        const int backIndex = 1 - m_frontIndex.loadRelaxed();
        //m_textures[backIndex]->setData(glFrame); //这里使用qt封装的setData会打印各种错误信息，但不影响显示
        m_textures[backIndex]->bind();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // 处理非4倍宽度
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        m_videoSize.width(), m_videoSize.height(),
                        GL_RGBA, GL_UNSIGNED_BYTE, glFrame.constBits());

        // 原子操作交换缓冲区
        m_frontIndex.storeRelaxed(backIndex);
        update();
        doneCurrent();
    }

}

QImage videoPlayOpengl::yv12ToQImage(const uint8_t *yv12Data, int width, int height)
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

void videoPlayOpengl::yv12ToImageCallBack(int nPort, char *pBuf, int nSize, FRAME_INFO *pFrameInfo, void *nUser, int nReserved2)
{
    qDebug()<<"nSize:"<<nSize<<pFrameInfo->nWidth<<pFrameInfo->nHeight<<pFrameInfo->nType<<pFrameInfo->nFrameRate;
    QImage frame = yv12ToQImage((uint8_t *)pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight);

    // 通过元调用系统保证线程安全（回到主线程）
    QMetaObject::invokeMethod(this, [this, frame]() {
        updateVideo(frame);
    }, Qt::QueuedConnection);
}


void videoPlayOpengl::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}
void videoPlayOpengl::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    if(m_program.bind()) {
        m_textures[m_frontIndex.loadRelaxed()]->bind(0);
        m_program.setUniformValue("texture_diffuse", 0);

        m_VAO.bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        m_VAO.release();

        m_program.release();
    }
}
