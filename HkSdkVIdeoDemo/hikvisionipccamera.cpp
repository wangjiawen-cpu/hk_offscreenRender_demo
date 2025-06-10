#include "hikvisionipccamera.h"
#include "include/basic.h"
#include "include/hikvision/HCNetSDK.h"
#include "include/hikvision/LinuxPlayM4.h"
#include <QDateTime>
#include "checkNetwork.h"

YUV_ImageCallBack g_YuvCallBack = NULL;

// PTZ 按照海康的格式转换成float
static float convertToInt(int in)
{
    char hexVal[12];
    memset(hexVal,0,12);
    sprintf(hexVal,"%04x",in);
    int val = atoi(hexVal) / 10;
    return val;
}

static float convertToIntT(int in)
{
    char hexVal[12];
    memset(hexVal,0,12);
    sprintf(hexVal,"%04x",in);
    int val = atoi(hexVal) / 10;
    if(val <= 90)  // 正值
    {
        return val;
    }
    return (360 - val) * -1;
}


static int  StringHexToInt(char* str)
{
    //assert(str);
    int result = 0;
    int flag = 0;

    if (*str == '-')
    {
        flag = 1;
        str++;
    }

    while (*str)
    {
        if (*str >= '0' && *str <= '9')
            result = result * 16 + (*str - '0');
        else if (*str >= 'a' && *str <= 'f')
            result = result * 16 + (*str - 'a' + 10);
        else if (*str >= 'A' && *str <= 'F')
            result = result * 16 + (*str - 'A' + 10);
        else {
            //  cout << "非法字符！" << endl;
            return 0;
        }
        str++;
    }

    if (flag == 1) result = -result;

    return result;
}

static unsigned short convertToValT(int in)
{
    char buf[12];
    memset(buf,0,12);

    if(in > 0)
    {
        sprintf(buf,"%d",in*10);
        int calVal = StringHexToInt(buf);
        return (unsigned short)calVal;
    }

    int val = 3600 + in * 10;
    sprintf(buf,"%d",val);
    int calVal = StringHexToInt(buf);
    return (unsigned short)calVal;
}

static unsigned short convertToVal(int in)
{
    char buf[12];
    memset(buf,0,12);
    sprintf(buf,"%d",in * 10);
    int calVal = StringHexToInt(buf);
    return (unsigned short)calVal;
}

// 图像帧回调
void imageFrameCallBack(int nPort,char * pBuf,int nSize,FRAME_INFO * pFrameInfo, void* nUser,int nReserved2)
{
    if(g_YuvCallBack != NULL)
    {
        g_YuvCallBack(nPort,pBuf,nSize,pFrameInfo,nUser,nReserved2);
    }
}

void CALLBACK g_RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer,DWORD dwBufSize,void* dwUser)
{
    int m_iPort = -1;
    MY_NET_DVR_CLIENTINFO* pInfo = (MY_NET_DVR_CLIENTINFO*) dwUser;

    //printf("    pInfo->lPort = %d , pInfo->hPlayWnd = %d\n", pInfo->lPort,pInfo->hPlayWnd);

    switch (dwDataType)
    {
    case NET_DVR_SYSHEAD: //系统头
    {
        if (pInfo->lPort >= 0)
        {
            break;  //该通道取流之前已经获取到句柄，后续接口不需要再调用
        }

        if (!PlayM4_GetPort(&m_iPort))  //获取播放库未使用的通道号
        {
            break;
        }
        pInfo->lPort = m_iPort;
        //m_iPort = lPort; //第一次回调的是系统头，将获取的播放库port号赋值给全局port，下次回调数据时即使用此port号播放
        if (dwBufSize > 0)
        {
            if (!PlayM4_SetStreamOpenMode(pInfo->lPort, STREAME_REALTIME))  //设置实时流播放模式
            {
                break;
            }

            //设置播放帧缓冲
            if (!PlayM4_SetDisplayBuf(pInfo->lPort, 4)) //打开流接口nNum参数范围：MIN_DIS_FRAMES ~MAX_DIS_FRAMES  越大越流畅但延时越高
            {
                LOG_OUTPUT_ERROR("PlayM4_SetDisplayBuf error!,nNum :%d",15);
                SLOG_ERROR("PlayM4_SetDisplayBuf  error!,nNum :{0}",15);  //帧缓冲只会影响流畅度和延时所以错误不用退出
            }

            // 1 视频流，2 音频流，3 复合流
            if(!PlayM4_SetDecCBStream(pInfo->lPort,1))
            {
                LOG_OUTPUT_ERROR("PlayM4_SetDecCBStream error!,nNum :%d",15);
                SLOG_ERROR("PlayM4_SetDecCBStream  error!,nNum :{0}",15);  //帧缓冲只会影响流畅度和延时所以错误不用退出
            }

            if(!PlayM4_SetDecCallBackMend(pInfo->lPort,imageFrameCallBack,NULL))
            {
                LOG_OUTPUT_ERROR("PlayM4_SetDecCallBackMend error!,nNum :%d",15);
                SLOG_ERROR("PlayM4_SetDecCallBackMend  error!,nNum :{0}",15);  //帧缓冲只会影响流畅度和延时所以错误不用退出
            }

            if (!PlayM4_OpenStream(pInfo->lPort, pBuffer, dwBufSize, 8 * 1024*1024)) //打开流接口
            {
                break;
            }

            //if (!PlayM4_Play(pInfo->lPort, pInfo->hPlayWnd))
            if (!PlayM4_Play(pInfo->lPort, pInfo->hPlayWnd)) //播放开始
            {
                break;
            }
        }
    }
        break;
    case NET_DVR_STREAMDATA:   //码流数据
        if (dwBufSize > 0 && pInfo->lPort != -1)
        {
            //加上硬解码会很卡
            if (!PlayM4_InputData(pInfo->lPort, pBuffer, dwBufSize))
            {
                qDebug() << "++++++++++码流数据(PlayM4_InputData)：返回0";
                break;
            }
        }
        break;
    default: //其他数据
        if (dwBufSize > 0 && pInfo->lPort != -1)
        {
            if (!PlayM4_InputData(pInfo->lPort, pBuffer, dwBufSize))
            {
                qDebug() << "++++++++++其他数据(PlayM4_InputData)：返回0";
                break;
            }
        }
        break;
    }
}

//实时预览回调数据
void CALLBACK dataCallBack_RealPlay(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* pUser)
{
    switch (dwDataType)
    {
    case NET_DVR_SYSHEAD: //系统头，打开流
    {//
        //打开声音
        bool  bRet = NET_DVR_OpenSound(lRealHandle);
        if (bRet == FALSE)
        {
            DWORD dwErrorCode = NET_DVR_GetLastError();
            LOG_OUTPUT_ERROR("NET_DVR_OpenSound error!,iLastErr:%d",dwErrorCode);
            SLOG_ERROR("NET_DVR_OpenSound  error!,iLastErr:{0}",dwErrorCode);
        }

        //设置音量
        WORD wTemp = (0xFFFF) / 100;
        WORD wVolumevalue = (WORD)(50 * wTemp);
        bRet = NET_DVR_Volume(lRealHandle, wVolumevalue);
        if (bRet == FALSE)
        {
            DWORD dwErrorCode = NET_DVR_GetLastError();
            LOG_OUTPUT_ERROR("NET_DVR_Volume error!,iLastErr:%d",dwErrorCode);
            SLOG_ERROR("NET_DVR_Volume  error!,iLastErr:{0}",dwErrorCode);
        }
    }
        break;
    }
}

HikvisionIpcCamera::HikvisionIpcCamera()
{
    m_userId = -1;
    m_playId = -1;
    memset(&m_clientInfo,0,sizeof(m_clientInfo));
}

HikvisionIpcCamera::~HikvisionIpcCamera()
{

}

void HikvisionIpcCamera::init(const SdkIpcLoginInfo &inf)
{
    memset(&m_loginInfo,0,sizeof(m_loginInfo));
    m_loginInfo = inf;
}

int HikvisionIpcCamera::login(const SdkIpcLoginInfo & inf)
{
    int rUserId = -1;

    //注册设备
    NET_DVR_USER_LOGIN_INFO stLoginInfo;
    NET_DVR_DEVICEINFO_V40 stDeviceInfo;
    memset(&stLoginInfo,0x00,sizeof(NET_DVR_USER_LOGIN_INFO));
    memset(&stDeviceInfo,0x00,sizeof(NET_DVR_DEVICEINFO_V40));

    stLoginInfo.wPort = inf.port;

    memset(&m_clientInfo,0,sizeof(m_clientInfo));


    //ip address
    strncpy(stLoginInfo.sDeviceAddress,inf.ip,sizeof(stLoginInfo.sDeviceAddress));
    //username
    strncpy(stLoginInfo.sUserName,inf.user,sizeof(stLoginInfo.sUserName));
    //password
    strncpy(stLoginInfo.sPassword,inf.password,sizeof(stLoginInfo.sPassword));

    //同步登录，登录接口返回成功，即登录成功
    stLoginInfo.bUseAsynLogin = 0;

    rUserId = NET_DVR_Login_V40(&stLoginInfo,&stDeviceInfo);
    if(rUserId < 0)
    {
        DWORD errCode = NET_DVR_GetLastError();
        LOG_OUTPUT_ERROR("registerInfo() NET_DVR_Login_V40  error!,ip:%s,port:%d,error code:%d",inf.ip,inf.port,errCode);
        SLOG_ERROR("registerInfo() NET_DVR_Login_V40  error!,ip:{0},port:{1},error code:{2}",inf.ip,inf.port,errCode);

        return fSPC_FAILURE;
    }

    LOG_OUTPUT_INFO("registerInfo() NET_DVR_Login_V40  ok!,rUserId:%d",rUserId);

    m_userId =  rUserId;
    m_clientInfo.lUserID = rUserId;
    m_bIsLogin = true;

    return fSPC_OK;
}

int HikvisionIpcCamera::startPlayer(const SdkStartPlayInfo &inf)
{
    int handle = 0;

    NET_DVR_PREVIEWINFO struPlayInfo;
    memset(&struPlayInfo,0x00,sizeof(NET_DVR_PREVIEWINFO));
    //显示窗口ID
    //struPlayInfo.hPlayWnd = inf.frameWnd;
    //显示通道号
    struPlayInfo.lChannel = inf.chId;
    //0-主码流，1-子码流，2-码流 3，3-码流 4，以此类推
    struPlayInfo.dwStreamType = inf.stream;
    //0- TCP 方式，1- UDP 方式，2- 多播方式，3- RTP 方式，4-RTP/RTSP，5-RSTP/HTTP
    struPlayInfo.dwLinkMode = 4;//0;
    //0- 非阻塞取流，1- 阻塞取流
    struPlayInfo.bBlocked = 1;
    //播放库播放缓冲区最大缓冲帧数
    struPlayInfo.dwDisplayBufNum = 5;

    if(inf.decType == SDK_IPC_DEC_GPU)
    {
        m_clientInfo.mIsCallback = true;
        m_clientInfo.lPort = -1;
        //m_clientInfo.hPlayWnd = inf.frameWnd;
    }

    //使用回调函数显示画面
    if(m_clientInfo.mIsCallback)
    {
        handle = NET_DVR_RealPlay_V40(m_userId, &struPlayInfo, g_RealDataCallBack_V30, &m_clientInfo);
    }
    else if(inf.voiceEnable)
    {
        handle = NET_DVR_RealPlay_V40(m_userId, &struPlayInfo, dataCallBack_RealPlay,NULL);
    }
    else
    {
        handle = NET_DVR_RealPlay_V40(m_userId, &struPlayInfo, NULL, NULL);
    }

    if(handle < 0)
    {
        LOG_OUTPUT_ERROR("registerInfo() NET_DVR_RealPlay_V40  error!,chNum:%d,userId:%d",inf.chId,m_userId);
        SLOG_ERROR("registerInfo() NET_DVR_RealPlay_V40  error!,chNum:{0},userId:{1}",inf.chId,m_userId);
        NET_DVR_Logout(m_userId);
        return fSPC_FAILURE;
    }

    LOG_OUTPUT_INFO("registerInfo() NET_DVR_RealPlay_V40  ok!,handle:%d",handle);
    m_playId =  handle;
    m_playInfo = inf;
    m_bIsPlaying = true;
    return fSPC_OK;
}

void HikvisionIpcCamera::stopPlayer()
{
    if(m_playId >= 0)
    {
        bool ret = NET_DVR_StopRealPlay(m_playId);
        if(!ret)
        {
            LOG_OUTPUT_ERROR("HikvisionIpcCamera::stopPlayer() error");
        }
    }

    if(m_clientInfo.mIsCallback)
    {
        PlayM4_Stop(m_clientInfo.lPort);
        PlayM4_CloseStream(m_clientInfo.lPort);
        PlayM4_FreePort(m_clientInfo.lPort);
        memset(&m_clientInfo,0,sizeof(m_clientInfo));
    }

    memset(&m_playInfo,0,sizeof(m_playInfo));
    m_playId = -1;
    m_bIsPlaying = false;
}

void HikvisionIpcCamera::logout()
{
    NET_DVR_Logout(m_userId);
    m_userId = -1;
    m_bIsLogin = false;
}

void HikvisionIpcCamera::closeMoveMonitor()
{
    int p = NET_DVR_GetRealPlayerIndex(m_userId);
    PlayM4_RenderPrivateData(p,2,false);//取消移动侦测功能
    LOG_OUTPUT_INFO("close_move_func() PlayM4_RenderPrivateData  ok!");
}

int HikvisionIpcCamera::setDateTimeCurrent()
{
    //    NET_DVR_TIME m_structTime;
    //    memset(&m_structTime,0x00,sizeof(NET_DVR_TIME));
    //    QDateTime curTime = QDateTime::currentDateTime();

    //    curTime = curTime.addSecs(2);

    //    m_structTime.dwYear = curTime.date().year();
    //    m_structTime.dwMonth = curTime.date().month();
    //    m_structTime.dwDay = curTime.date().day();
    //    m_structTime.dwHour = curTime.time().hour();
    //    m_structTime.dwMinute = curTime.time().minute();
    //    m_structTime.dwSecond = curTime.time().second();


    //    uint dwSize =sizeof(NET_DVR_TIME);

    //    int iGroupNo =0xFFFFFFFF;

    //    if(!NET_DVR_SetDVRConfig(m_userId,NET_DVR_SET_TIMECFG, iGroupNo, &m_structTime, dwSize))
    //    {
    //        uint iLastErr =NET_DVR_GetLastError();
    //        LOG_OUTPUT_ERROR("NET_DVR_SET_TIMECFG error!,iLastErr:%d",iLastErr);
    //        SLOG_ERROR("NET_DVR_SET_TIMECFG  error!,iLastErr:{0}",iLastErr);
    //        return fSPC_FAILURE;
    //    }
    //    LOG_OUTPUT_INFO("NET_DVR_SetDVRConfig ok!");
    //    return fSPC_OK;




    NET_DVR_CORRIDOR_MODE m_structCorriDor;
    memset(&m_structCorriDor,0x00,sizeof(NET_DVR_CORRIDOR_MODE));

    m_structCorriDor.dwSize = sizeof(NET_DVR_CORRIDOR_MODE);
    m_structCorriDor.byEnableCorridorMode = 1;
    m_structCorriDor.byMirrorMode = 2;

    int iGroupNo =2;
    unsigned int count = 0;

    if(!NET_DVR_SetDVRConfig(m_userId,NET_DVR_SET_CORRIDOR_MODE, iGroupNo, &m_structCorriDor, sizeof(NET_DVR_CORRIDOR_MODE)))
    {
        uint iLastErr =NET_DVR_GetLastError();
        LOG_OUTPUT_ERROR("NET_DVR_SET_CORRIDOR_MODE error!,iLastErr:%d",iLastErr);
        SLOG_ERROR("NET_DVR_SET_CORRIDOR_MODE  error!,iLastErr:{0}",iLastErr);
        return fSPC_FAILURE;
    }
    LOG_OUTPUT_INFO("NET_DVR_SET_CORRIDOR_MODE ok!");

    memset(&m_structCorriDor,0x00,sizeof(NET_DVR_CORRIDOR_MODE));
    if(!NET_DVR_GetDVRConfig(m_userId,NET_DVR_GET_CORRIDOR_MODE, iGroupNo, &m_structCorriDor, sizeof(NET_DVR_CORRIDOR_MODE), &count))
    {
        uint iLastErr =NET_DVR_GetLastError();
        LOG_OUTPUT_ERROR("NET_DVR_GET_CORRIDOR_MODE error!,iLastErr:%d",iLastErr);
        SLOG_ERROR("NET_DVR_GET_CORRIDOR_MODE  error!,iLastErr:{0}",iLastErr);
    }

    return fSPC_OK;
}

/**
* @description 设置音量
* @param vol 设置音量百分比 ）0.01 ~ 1
* @return fSPC_OK or fSPC_FAILURE
*/
int HikvisionIpcCamera::setVolumn(float vol)
{
    //设置音量
    //    WORD wTemp = (0xFFFF) * vol;
    //    WORD wVolumevalue = wTemp;
    WORD wTemp = (0xFFFF) / 100;
    WORD wVolumevalue = (WORD)(50 * wTemp);
    bool bRet = NET_DVR_Volume(m_playId, wVolumevalue);
    if (bRet == FALSE)
    {
        DWORD dwErrorCode = NET_DVR_GetLastError();
        LOG_OUTPUT_ERROR("NET_DVR_Volume error!,iLastErr:%d",dwErrorCode);
        SLOG_ERROR("NET_DVR_Volume  error!,iLastErr:{0}",dwErrorCode);
        return fSPC_FAILURE;
    }
    LOG_OUTPUT_INFO("NET_DVR_Volume ok!");
    return fSPC_OK;
}

int HikvisionIpcCamera::setSoundEnable(bool open)
{
    if(m_playId < 0) return fSPC_FAILURE;
    if(open)
    {
        bool  bRet = NET_DVR_OpenSound(m_playId);
        if(bRet == FALSE)
        {
            uint iLastErr =NET_DVR_GetLastError();
            LOG_OUTPUT_ERROR("NET_DVR_OpenSound error!,iLastErr:%d",iLastErr);
            SLOG_ERROR("NET_DVR_OpenSound  error!,iLastErr:{0}",iLastErr);
            return fSPC_FAILURE;
        }
        LOG_OUTPUT_INFO("NET_DVR_OpenSound enable ok!");
    }
    else
    {
        NET_DVR_CloseSound();
        LOG_OUTPUT_INFO("NET_DVR_OpenSound disabled ok!");
    }
    return fSPC_OK;
}

int HikvisionIpcCamera::setZoomIn(int st,float speed)
{
    int dwStop = st == SDK_RELATIVE_STOP_MOVE ? 1 : 0;
    if(NET_DVR_PTZControl(m_playId,ZOOM_IN,dwStop))
        return fSPC_OK;
    LOG_OUTPUT_ERROR("HikvisionIpcCamera::setZoomIn error!");
    return fSPC_FAILURE;
}
int HikvisionIpcCamera::setZoomOut(int st,float speed)
{
    int dwStop = st == SDK_RELATIVE_STOP_MOVE ? 1 : 0;
    if(NET_DVR_PTZControl(m_playId,ZOOM_OUT,dwStop))
        return fSPC_OK;
    LOG_OUTPUT_ERROR("tilt_up_hk error,error code:%d",NET_DVR_GetLastError());
    return fSPC_FAILURE;
}
int HikvisionIpcCamera::setTiltUp(int st,float speed)
{
    int dwStop = st == SDK_RELATIVE_STOP_MOVE ? 1 : 0;
    if(NET_DVR_PTZControl(m_playId,TILT_UP,dwStop))
        return fSPC_OK;
    LOG_OUTPUT_ERROR("tilt_up_hk error,error code:%d",NET_DVR_GetLastError());
    return fSPC_FAILURE;
}
int HikvisionIpcCamera::setTiltDown(int st,float speed)
{
    int dwStop = st == SDK_RELATIVE_STOP_MOVE ? 1 : 0;
    if(NET_DVR_PTZControl(m_playId,TILT_DOWN,dwStop))
        return fSPC_OK;
    LOG_OUTPUT_ERROR("tilt_down_hk error,error code:%d",NET_DVR_GetLastError());
    return fSPC_FAILURE;
}
int HikvisionIpcCamera::setPanLeft(int st,float speed)
{
    int dwStop = st == SDK_RELATIVE_STOP_MOVE ? 1 : 0;
    if(NET_DVR_PTZControl(m_playId,PAN_LEFT,dwStop))
        return fSPC_OK;
    LOG_OUTPUT_ERROR("pan_left_hk error,error code:%d",NET_DVR_GetLastError());
    return fSPC_FAILURE;
}
int HikvisionIpcCamera::setPanRight(int st,float speed)
{
    int dwStop = st == SDK_RELATIVE_STOP_MOVE ? 1 : 0;
    if(NET_DVR_PTZControl(m_playId,PAN_RIGHT,dwStop))
        return fSPC_OK;
    LOG_OUTPUT_ERROR("pan_right_hk error,error code:%d",NET_DVR_GetLastError());
}

int HikvisionIpcCamera::getPtz(int *p,int *t,int *z)
{
    NET_DVR_PTZPOS struPtzPos = {0};
    DWORD dwReturn = 0;
    int chId = getChId();
    if(NET_DVR_GetDVRConfig(m_userId, NET_DVR_GET_PTZPOS, chId, &struPtzPos, sizeof(NET_DVR_PTZPOS), &dwReturn))
    {
        *p = convertToInt(struPtzPos.wPanPos);
        *t = convertToIntT(struPtzPos.wTiltPos);
        *z = convertToInt(struPtzPos.wZoomPos);
        return fSPC_OK;
    }
    LOG_OUTPUT_ERROR("getPtz_hk error,error code:%d",NET_DVR_GetLastError());
    return fSPC_FAILURE;
}
int HikvisionIpcCamera::setPtz(int p,int t,int z,float speed,SdkPtzControl ct)
{
    NET_DVR_PTZPOS struPtzPos = {0};
    int chId = getChId();

    if(ct == SDK_PTZ_CONTROL_P)
        struPtzPos.wAction = 2; // 控制P
    else if(ct == SDK_PTZ_CONTROL_T)
        struPtzPos.wAction = 3;
    else if(ct == SDK_PTZ_CONTROL_Z)
        struPtzPos.wAction = 4;
    else
        struPtzPos.wAction = 1; // 控制PTZ

    struPtzPos.wPanPos = convertToVal(p);
    struPtzPos.wTiltPos = convertToValT(t);
    struPtzPos.wZoomPos = convertToVal(z);

    if(NET_DVR_SetDVRConfig(m_userId, NET_DVR_SET_PTZPOS,chId, &struPtzPos, sizeof(struPtzPos)))
        return fSPC_OK;
    LOG_OUTPUT_ERROR("setPtz_hk error,error code:%d",NET_DVR_GetLastError());
    return fSPC_FAILURE;
}

void HikvisionIpcCamera::updateNetState()
{
    if(strlen(m_loginInfo.ip) == 0 || m_loginInfo.port == 0)
    {
        m_bNetIsOk = false;
        return;
    }

    m_bNetIsOk =  checkVideoConnect((unsigned char *)m_loginInfo.ip,m_loginInfo.port,500);
}

void HikvisionIpcCamera::updateNetDelay()
{
    if(strlen(m_loginInfo.ip) == 0 || m_loginInfo.port == 0)
    {
        m_bNetIsOk = false;
        return;
    }
    m_netDelay = caleVideoDelay(m_loginInfo.ip,m_loginInfo.port);
}

void HikvisionIpcCamera::setYuVCallBack(YUV_ImageCallBack func)
{
    g_YuvCallBack = func;
}

