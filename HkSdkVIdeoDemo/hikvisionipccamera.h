/*************************************************
Copyright:fosow
Author:wangyanbo
Date:2025-01-20
Description:海康IPC类
**************************************************/

#ifndef HIKVISIONIPCCAMERA_H
#define HIKVISIONIPCCAMERA_H


#include "include/abs/IpcSdkAbstract.h"
#include "include/hikvision/HCNetSDK.h"
#include "include/hikvision/PlayM4.h"
#include <QDebug>

//软解码预览参数
typedef struct MY_NET_DVR_CLIENTINFO_t
{
    bool mIsCallback;
    LONG lPort;
    //启动预览并设置回调数据流
    LONG lRealPlayHandle;
    LONG lUserID;

    LONG lChannel;//通道号
    LONG lLinkMode;//最高位(31)为0表示主码流，为1表示子，0－30位表示码流连接方式: 0：TCP方式,1：UDP方式,2：多播方式,3 - RTP方式，4-RTP/RTSP,5-RSTP/HTTP
    HWND hPlayWnd;//播放窗口的句柄,为NULL表示不播放图象
    char* sMultiCastIP;//多播组地址
    BYTE byProtoType; //应用层取流协议，0-私有协议，1-RTSP协议
    BYTE byRes[3];
} MY_NET_DVR_CLIENTINFO;


#define MAX_PALY_NUM    12

//typedef void(*YUV_ImageCallBack)(int nPort,char * pBuf,int nSize,FRAME_INFO * pFrameInfo, void* nUser,int nReserved2);
using YUV_ImageCallBack = std::function<void(int nPort,char * pBuf,int nSize,FRAME_INFO * pFrameInfo, void* nUser,int nReserved2)>;

class HikvisionIpcCamera:public IpcSdkAbstract
{

private:
    int m_userId;  // 用户ID
    int m_playId;  // 播放ID

    MY_NET_DVR_CLIENTINFO m_clientInfo;
    YUV_ImageCallBack m_YuvCallBack;

public:
    HikvisionIpcCamera();
    ~HikvisionIpcCamera();
    void init(const SdkIpcLoginInfo &inf);
    int login(const SdkIpcLoginInfo & inf);
    int startPlayer(const SdkStartPlayInfo &inf);
    void stopPlayer();
    void logout();
    void updateNetState();
    void updateNetDelay();

    void closeMoveMonitor();
    int setDateTimeCurrent();
    int setVolumn(float vol);
    int setSoundEnable(bool open);

    int setZoomIn(int st,float speed =0);
    int setZoomOut(int st,float speed =0);
    int setTiltUp(int st,float speed =0);
    int setTiltDown(int st,float speed =0);
    int setPanLeft(int st,float speed =0);
    int setPanRight(int st,float speed =0);

    int getPtz(int *p,int *t,int *z);
    int setPtz(int p,int t,int z,float speed,SdkPtzControl ct);
    void setYuVCallBack(YUV_ImageCallBack func);
};

#endif // HIKVISIONIPCCAMERA_H
