#include "hikvisionsdk.h"
#include "include/hikvision/HCNetSDK.h"
#include "include/hikvision/LinuxPlayM4.h"
#include "include/basic.h"

#define HIKVISION_CONN_TIME             2000        //海康摄像头连接时间
#define HIKVISION_RECONN_TIME           10000       //海康摄像头重连时间

HikVisionSdk::HikVisionSdk()
{

}

HikVisionSdk::~HikVisionSdk()
{

}

/**
* @description 初始化海康SDK
* @return fSPC_OK or fSPC_FAILURE
*/
int HikVisionSdk::initSdk()
{
    bool bInit = NET_DVR_Init();
    if(!bInit)
    {
        LOG_OUTPUT_ERROR("initHikvisonSDK() NET_DVR_Init  error!");
        SLOG_ERROR("initHikvisonSDK() NET_DVR_Init  error!");
        return fSPC_FAILURE;
    }

    //设置连接时间与重连时间
    NET_DVR_SetConnectTime(HIKVISION_CONN_TIME,1);
    NET_DVR_SetReconnect(HIKVISION_RECONN_TIME,true);

    return fSPC_OK;
}

/**
* @description 反初始化SDK
*/
void HikVisionSdk::unInitSdk()
{
    NET_DVR_Cleanup();
}
