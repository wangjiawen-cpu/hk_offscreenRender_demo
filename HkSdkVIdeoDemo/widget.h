#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "hikvisionipccamera.h"
#include "videoplayopengl.h"
#include "videowidget.h"
#include <QHBoxLayout>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    int init();

private slots:
    void on_pbLoginout_clicked();
    void on_pbPlayStop_clicked();

    void on_pbInit_clicked();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::Widget *ui;
    void unInit();
    HikvisionIpcCamera *m_pIpc;
    HikvisionIpcCamera *m_pIpc2;
    videoPlayOpengl *m_videoPlayOpengl;
    VideoWidget *m_videoWidget;
    QHBoxLayout * m_hLayout;

};
#endif // WIDGET_H
