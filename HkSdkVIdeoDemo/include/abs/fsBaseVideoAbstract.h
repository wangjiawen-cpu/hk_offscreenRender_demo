#ifndef FSBASEVIDEOABSTRACT_H
#define FSBASEVIDEOABSTRACT_H

enum FS_PTZ_CONTROL_SELECT
{
    FS_PTZ_CONTROL_PTZ = 0,
    FS_PTZ_CONTROL_P = 1,
    FS_PTZ_CONTROL_T = 2,
    FS_PTZ_CONTROL_Z = 3,
};

//摄像头抽象类
class fsBaseVideoAbstract
{
public:
    fsBaseVideoAbstract()
    {
       m_bInit = false;
    }
    virtual ~fsBaseVideoAbstract(){};

    virtual int init(const void *para) = 0;
    virtual int unInit() = 0;
    virtual int login_device(const char* user,const char * pwd,const char* ip, unsigned int port) = 0;
    virtual int open_player(unsigned int frameId,unsigned int chId) = 0;
    virtual void close_player() = 0;
    virtual void zoom_in() = 0;
    virtual void zoom_out() = 0;
protected:
    bool m_bInit;
};

#endif // FSBASEVIDEOABSTRACT_H
