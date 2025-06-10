/*************************************************
Copyright:fosow
Author:wangyanbo
Date:2024-12-18
Description:海康SDK包的操作类
**************************************************/

#ifndef HIKVISIONSDK_H
#define HIKVISIONSDK_H


class HikVisionSdk
{
public:
    HikVisionSdk();
    ~HikVisionSdk();
    HikVisionSdk(const HikVisionSdk &);
    HikVisionSdk &operator=(const HikVisionSdk &);

    static HikVisionSdk &getInstance()
    {
        static HikVisionSdk instance;
        return instance;
    }

    int initSdk();
    void unInitSdk();
};

#endif // HIKVISIONSDK_H
