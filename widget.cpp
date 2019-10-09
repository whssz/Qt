#include "widget.h"
#include <QDebug>
#include <windows.h>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    createView();
}

void Widget::createView()
{
    /*添加界面*/
    QPushButton *openThreadBtn = new QPushButton(tr("打开线程"));
    QPushButton *closeThreadBtn = new QPushButton(tr("关闭线程"));
    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(openThreadBtn);
    mainLayout->addWidget(closeThreadBtn);
    mainLayout->addStretch();
    connect(openThreadBtn,SIGNAL(clicked(bool)),this,SLOT(openThreadBtnSlot()));
    connect(closeThreadBtn,SIGNAL(clicked(bool)),this,SLOT(closeThreadBtnSlot()));

    /*线程初始化*/
    threadArray[0] = new MyThread;
    threadArray[1] = new MyThread;
    connect(threadArray[0] ,&QThread::finished,this,&Widget::finishedThreadBtnSlot); //打印线程结束信息
    connect(threadArray[1] ,&QThread::finished,this,&Widget::finishedThreadBtnSlot);
}
void Widget::openThreadBtnSlot()
{
    /*开启一个线程*/
    threadArray[0]->start();
    threadArray[1]->start();
    qDebug()<<"主线程id："<<QThread::currentThreadId();
}
void Widget::closeThreadBtnSlot()
{
    /*关闭多线程*/
    threadArray[0]->closeThread();
    threadArray[1]->closeThread();
    threadArray[0]->wait();
    threadArray[1]->wait();
}

void Widget::finishedThreadBtnSlot()
{
    qDebug()<<tr("完成信号finished触发");
}

Widget::~Widget()
{

}
