#include "sharedglcontext.h"

VideoSharedGLContext::VideoSharedGLContext()
{
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer); // 双缓冲

    // 初始化主上下文
    m_context.setFormat(format);
    m_context.create();

    // 初始化离屏表面
    m_surface.setFormat(format);
    m_surface.create();
}
