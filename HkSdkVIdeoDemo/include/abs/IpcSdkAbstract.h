/*************************************************
Copyright:fosow
Author:wangyanbo
Date:2024-12-18
Description:SDK的IPC的抽象类
**************************************************/

#ifndef IPCSDKABSTRACT_H
#define IPCSDKABSTRACT_H

#include "../IpcSdkDef.h"




class IpcSdkAbstract
{

private:

protected:
    SdkIpcLoginInfo m_loginInfo;
    SdkStartPlayInfo m_playInfo;
    bool m_bIsLogin;
    bool m_bNetIsOk;
    bool m_bIsPlaying;
    int m_netDelay; //网络时延

public:
    IpcSdkAbstract()
    {
        memset(&m_loginInfo,0,sizeof(m_loginInfo));
        memset(&m_playInfo,0,sizeof(m_playInfo));
        m_bIsLogin = false;
        m_bNetIsOk = false;
        m_bIsPlaying = false;
        m_netDelay = 0;
    };

    virtual ~IpcSdkAbstract(){};

    int getChId(){return m_playInfo.chId;}  // 获取通道ID
    int getIpcType(){return m_loginInfo.type;} // 获取IPC的厂商信息
    bool isLogin(){return m_bIsLogin;}
    bool isNetOk(){return m_bNetIsOk;} // 返回网络是否可达
    bool isPlaying(){return m_bIsPlaying;} //是否启动了预览
    int getNetDelay(){return m_netDelay;}
    virtual void init(const SdkIpcLoginInfo & inf)= 0;
    virtual int login(const SdkIpcLoginInfo & inf) = 0;
    virtual int startPlayer(const SdkStartPlayInfo &inf) = 0;
    virtual void stopPlayer() = 0;
    virtual void logout() = 0;
    virtual void updateNetState() = 0;
    virtual void updateNetDelay() = 0;

    virtual void closeMoveMonitor() = 0;
    virtual int setDateTimeCurrent() = 0;
    virtual int setVolumn(float vol) = 0;
    virtual int setSoundEnable(bool open) = 0;

    virtual int setZoomIn(int st,float speed =0) = 0;
    virtual int setZoomOut(int st,float speed =0) = 0;
    virtual int setTiltUp(int st,float speed =0) = 0;
    virtual int setTiltDown(int st,float speed =0) = 0;
    virtual int setPanLeft(int st,float speed =0) = 0;
    virtual int setPanRight(int st,float speed =0) = 0;

    virtual int getPtz(int *p,int *t,int *z) = 0;
    virtual int setPtz(int p,int t,int z,float speed,SdkPtzControl ct) = 0;

};


#endif // IPCSDKABSTRACT_H
