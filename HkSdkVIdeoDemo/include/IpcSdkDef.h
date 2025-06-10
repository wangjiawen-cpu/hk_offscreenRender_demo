#ifndef IPCSDKDEF_H
#define IPCSDKDEF_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define SDK_IPC_LOGIN_USER_LEN  32
#define SDK_IPC_LOGIN_PWD_LEN   32
#define SDK_IPC_LOGIN_IP_LEN    16

// SDK IPC的类型
enum SdkIpcType
{
    SDK_IPC_HIKVISON = 1,
    SDK_IPC_UNIVEW = 2,
};

// SDK IPC的硬解码
enum SdkIpcDecodingType
{
    SDK_IPC_DEC_CPU = 0,  //SDK软解码
    SDK_IPC_DEC_GPU = 1, //GPU硬解码
};

//SDK IPC的登录信息
struct SdkIpcLoginInfo
{
    int type; //SdkIpcType
    char user[SDK_IPC_LOGIN_USER_LEN];
    char password[SDK_IPC_LOGIN_PWD_LEN];
    char ip[SDK_IPC_LOGIN_IP_LEN];
    unsigned int port;
};

//SDK 播放信息
struct SdkStartPlayInfo
{
    int chId; //通道ID
    unsigned int frameWnd; //窗口的ID
    bool voiceEnable;
    int stream; //码流 海康的定义 0-主码流，1-子码流，2-码流 3，3-码流 4，以此类推
    int asId; //对外关联的ID，和其他接口关联的ID
    int decType;//解码方式，SdkIpcDecodingType
};

//SDK 相对移动控制
enum SdkRelativeMoveControl
{
    SDK_RELATIVE_START_MOVE = 0,
    SDK_RELATIVE_STOP_MOVE = 1,
};

//SDK 云台控制选项
enum SdkPtzControl
{
    SDK_PTZ_CONTROL_PTZ = 0,
    SDK_PTZ_CONTROL_P = 1,
    SDK_PTZ_CONTROL_T = 2,
    SDK_PTZ_CONTROL_Z = 3,
};


#endif // IPCSDKDEF_H
