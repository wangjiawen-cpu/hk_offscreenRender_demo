#ifndef VIDEOSHAREDGLCONTEXT_H
#define VIDEOSHAREDGLCONTEXT_H

#include <QOpenGLContext>
#include <QOffscreenSurface>

// 共享上下文类，单例，全局唯一的上下文
class VideoSharedGLContext
{
    Q_OBJECT
public:
    static VideoSharedGLContext& instance()
    {
        static VideoSharedGLContext instance;
        return instance;
    }

    QOpenGLContext* context() { return &m_context; }
    QOffscreenSurface* surface() { return &m_surface; }

private:
    VideoSharedGLContext();
    QOpenGLContext m_context;
    QOffscreenSurface m_surface;

};

#endif // VIDEOSHAREDGLCONTEXT_H
