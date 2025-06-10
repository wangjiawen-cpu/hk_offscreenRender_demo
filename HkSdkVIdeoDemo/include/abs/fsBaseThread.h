#ifndef FSTHREAD_H
#define FSTHREAD_H

#include <QThread>

class fsBaseThread : public QThread
{
public:

    fsBaseThread()
    {
        m_bIsInit = false;
        m_bQuit = false;
        m_bIsRun = false;
    }

    ~fsBaseThread(){};

    virtual int init(const void *para) = 0;
    virtual void unInit() = 0;
    virtual int startThread() = 0;
    virtual void stopThread() = 0;

protected:
    std::atomic<bool> m_bQuit;  // 线程退出的标记
    std::atomic<bool> m_bIsRun; // 线程运行中状态标记
    bool m_bIsInit;
};

#endif // FSTHREAD_H
