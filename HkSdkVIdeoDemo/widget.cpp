#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include "hikvisionsdk.h"
#include "hikvisionipccamera.h"
#include <QMessageBox>
#include <iostream>

#define HIKVISION_CONN_TIME             2000        //海康摄像头连接时间
#define HIKVISION_RECONN_TIME           10000       //海康摄像头重连时间

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    m_pIpc = new HikvisionIpcCamera();

    m_videoWidget = new VideoWidget(this);
    m_videoWidget->show();  //这里使用show会和qt的渲染机制冲突
    m_hLayout = new QHBoxLayout(this);
    m_hLayout->addWidget(m_videoWidget);
    ui->frame->setLayout(m_hLayout);
}

int Widget::init()
{
    return HikVisionSdk::getInstance().initSdk();
}

void Widget::unInit()
{
    HikVisionSdk::getInstance().unInitSdk();
}

Widget::~Widget()
{
    unInit();
    delete m_pIpc;
    delete ui;
}

void Widget::on_pbInit_clicked()
{
    if(m_videoWidget)
    {
        m_pIpc->setYuVCallBack(m_videoWidget->getCallbackPointer());
    }

    SdkIpcLoginInfo lgInfo;
    memset(&lgInfo,0,sizeof(lgInfo));
    sprintf(lgInfo.ip,"192.168.1.91");
    sprintf(lgInfo.user,"admin");
    sprintf(lgInfo.password,"Fosow.2007");
    lgInfo.port = 80;
    m_pIpc->init(lgInfo);


    SdkIpcLoginInfo lgInfo2;
    memset(&lgInfo2,0,sizeof(lgInfo2));
    sprintf(lgInfo2.ip,"192.168.1.93");
    sprintf(lgInfo2.user,"admin");
    sprintf(lgInfo2.password,"Fosow.2007");
    lgInfo.port = 80;
    m_pIpc->init(lgInfo2);
}
void Widget::on_pushButton_2_clicked()
{
    if(!m_videoWidget)
    {
        m_videoWidget = new VideoWidget(this);
        //m_videoWindow->show();  //这里使用show会和qt的渲染机制冲突
        m_hLayout->addWidget(m_videoWidget);
        ui->frame->setLayout(m_hLayout);
    }
}
void Widget::on_pushButton_clicked()
{
    if(m_videoWidget)
    {
        m_pIpc->setYuVCallBack(nullptr);
        m_pIpc->logout();
        delete m_videoWidget;
        m_videoWidget = nullptr;
    }
}

void Widget::on_pbLoginout_clicked()
{
    //
    if(!m_pIpc->isLogin())
    {
        SdkIpcLoginInfo lgInfo;
        memset(&lgInfo,0,sizeof(lgInfo));
        sprintf(lgInfo.ip,"192.168.1.91");
        sprintf(lgInfo.user,"admin");
        sprintf(lgInfo.password,"Fosow.2007");
        lgInfo.port = 8000;
        int ret = m_pIpc->login(lgInfo);
        if(ret)
        {
            QMessageBox::information(this, "提示","Login失败！");
            return;
        }
        else
        {
            QMessageBox::information(this, "提示","Login成功！");
        }
    }
    else
    {
        m_pIpc->logout();
    }
return;
    //
    if(!m_pIpc2->isLogin())
    {
        SdkIpcLoginInfo lgInfo;
        memset(&lgInfo,0,sizeof(lgInfo));
        sprintf(lgInfo.ip,"192.168.1.93");
        sprintf(lgInfo.user,"admin");
        sprintf(lgInfo.password,"Fosow.2007");
        lgInfo.port = 8000;
        int ret = m_pIpc2->login(lgInfo);
        if(ret)
        {
            QMessageBox::information(this, "提示","Login失败！");
            return;
        }
        else
        {
            QMessageBox::information(this, "提示","Login成功！");
        }
    }
    else
    {
        m_pIpc2->logout();
    }
}

void Widget::on_pbPlayStop_clicked()
{
    //
    if(!m_pIpc->isPlaying())
    {
        if(!m_pIpc->isLogin())
        {
            QMessageBox::information(this, "提示","请先登录！");
            return;
        }

        SdkStartPlayInfo plInfo;
        memset(&plInfo,0,sizeof(plInfo));
        plInfo.frameWnd = 0;//ui->frame->winId();//this->winId();
        plInfo.chId = 1;
        plInfo.stream = 0;
        plInfo.decType = SDK_IPC_DEC_GPU;

        int ret = m_pIpc->startPlayer(plInfo);
        if(ret)
        {
            QMessageBox::information(this, "提示","启动预览失败！");
            return;
        }
        QMessageBox::information(this, "提示","启动成功！");
    }
    else
    {
        m_pIpc->stopPlayer();
        ui->frame->update();
    }
return;
    if(!m_pIpc2->isPlaying())
    {
        if(!m_pIpc2->isLogin())
        {
            QMessageBox::information(this, "提示","请先登录！");
            return;
        }

        SdkStartPlayInfo plInfo;
        memset(&plInfo,0,sizeof(plInfo));
        plInfo.frameWnd = 0;//ui->frame->winId();//this->winId();
        plInfo.chId = 1;
        plInfo.stream = 0;
        plInfo.decType = SDK_IPC_DEC_GPU;

        int ret = m_pIpc2->startPlayer(plInfo);
        if(ret)
        {
            QMessageBox::information(this, "提示","启动预览失败！");
            return;
        }
        QMessageBox::information(this, "提示","启动成功！");
    }
    else
    {
        m_pIpc2->stopPlayer();
        ui->frame->update();
    }
}




