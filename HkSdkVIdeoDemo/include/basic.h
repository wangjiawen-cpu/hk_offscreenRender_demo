#ifndef BASIC_H
#define BASIC_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifndef ushort
typedef unsigned short ushort;
#endif

#ifndef uchar
typedef unsigned char uchar;
#endif

#ifndef ulong
typedef unsigned int uint;
#endif

#ifndef ulong
typedef unsigned long ulong;
#endif


#ifndef fSPC_OK
#define fSPC_OK           (0)
#define fSPC_FAILURE      (-1)
#define fSPC_MSG_CRC_LEN  2
#define fSPC_MSG_HEAD_CODE (0xAADD) //消息头部
#define fSPC_MSG_DATA_MAX 4096
#define AI_BOX_SERVER_PORT 36889
#endif

// 接口调用的错误码
typedef enum fSPC_ErrorCode
{
    // - 1 是 fSPC_FALIURE 的返回值
    fSPC_ERROR_NOT_SUPPORT = -2,              // 不支持的接口
    fSPC_ERROR_DISK_WR_ERROR = -3,            // 磁盘出错
    fSPC_ERROR_FILE_OP_ERROR = -4,            // 文件操作出错
    fSPC_ERROR_INVALID_DATE = -5,             // 无效的时间
    fSPC_ERROR_DIR_NOT_EXIST = -6,            // 目录不存在
    fSPC_ERROR_INVALID_PARAM = -7,            // 无效的参数
    fSPC_ERROR_MAM_IS_NULL = -8,              // 内存申请为NULL
    fSPC_ERROR_NOT_INIT = -9,                 // 系统未初始化
    fSPC_ERROR_FILE_OPEN = -10,               // 文件没有打开
    fSPC_ERROR_FILE_NOT_EXITS = -11,          // 文件不存在
    fSPC_ERROR_CALL_IS_START = -12,           // 任务已启动，在执行中
    fSPC_ERROR_FILESYSTEM_OBJ_ERROR = -13,    // 文件系统对象出错
    fSPC_ERROR_DATA_IS_NULL = -14,            // 数据体为NULL
    fSPC_ERROR_SYS_DATA_FALIURE = -15,        // 系统数据出错
    fSPC_ERROR_SYS_RUN_FALIURE = -16,         // 系统运行出错
} fSPC_ErrorCode;


#define LOG_ERROR
#ifdef  LOG_ERROR
#define LOG_OUTPUT_ERROR(fmt...) \
    do {\
        printf("[%s-%s-%d]: ",__FILE__,__FUNCTION__, __LINE__);\
        printf(fmt);\
        printf("\n");\
       }while(0)
#else
#define LOG_OUTPUT_ERROR(fmt...)
#endif

#define LOG_WARN
#ifdef  LOG_WARN
#define LOG_OUTPUT_WARN(fmt...) \
    do {\
        printf("[%s-%s-%d]: ",__FILE__,__FUNCTION__, __LINE__);\
        printf(fmt);\
        printf("\n");\
       }while(0)
#else
#define LOG_OUTPUT_WARN(fmt...)
#endif


//条件编译实现日志打印 注释define可关闭
#define LOG_INFO
#ifdef  LOG_INFO
#define LOG_OUTPUT_INFO(fmt...) \
    do {\
        printf("[%s-%s-%d]: ",__FILE__,__FUNCTION__, __LINE__);\
        printf(fmt);\
        printf("\n");\
       }while(0)
#else
#define LOG_OUTPUT_INFO(fmt...)
#endif


#define SLOG_ERROR(fmt...)

#define QSS_PATH ":/style/resource/qss/" //qss路径

#pragma pack(1)
typedef struct _CraneNumTy
{//要存储下去的
    uchar num;			    // 编号（1~63）
    uchar craneType;		// 塔机类型
    ushort rsvd;            //默认角度，adjust by zk 2012-10-23 //2019-08-22 不用此参数
}CraneNumTy;
#pragma pack()

#define CHECK_NOTEQU_RET(ret, sret, a) \
        do{\
        if (ret != sret){\
            a;\
        }\
    }while(0);

#define CHECK_EQU_RET(ret, sret, a) \
        do{\
        if (ret == sret){\
            a;\
        }\
    }while(0);

#define SERVICES_ADDRESS_LENGTH 16

#define VIDEO_USERNAME_LEN      16

#define VIDEO_PWD_LEN       12

#define MAX_LIBRA_NUM   4   //

#define LIBRA_PORT_P1   9693
#define LIBRA_PORT_P2   9694
#define LIBRA_PORT_P3   9695
#define LIBRA_PORT_P4   9696

#define LIBRA_PORT_CONNECT   9697  //直接连接ip所使用的端口


#define BAISC_PARA_TIMEOUT 1000  /*基本参数发送间隔时间*/
#define HANDLE_INFO_TIMEOUT 100  /*手柄信息发送间隔时间*/

const int LibSystemPorts[MAX_LIBRA_NUM]{LIBRA_PORT_P1,LIBRA_PORT_P2,LIBRA_PORT_P3,LIBRA_PORT_P4};

// socket 信息结构体
typedef struct _SocketInfo
{
    char servicesAddress[SERVICES_ADDRESS_LENGTH]; //服务器地址
    uint sPort;//服务器端口号
    uint cPort;//客户端端口号
}SocketInfo;

#endif // BASIC_H
