#include "mythread.h"
#include "mainwindow.h"
#include <QDebug>
#include <QTime>
#include <QMutex>
#include <QPixmap>
#include "mythread.h"

VideoThread:: VideoThread(QObject *parent) //在函数实现时,不需要写入默认参数
   : QThread(parent)  //将其基类对象也接入相同的对象树中.
{
    decoder = new QFFmpeg(this); //QFFmpeg对象接入到thread中.
    connect(this, &VideoThread::stopDecode, decoder, &QFFmpeg::Stop);
}


void VideoThread::run()
{
    qDebug() << "线程运行 :" << this->currentThreadId();
    unsigned int threadID = getThreadId();
    //循环播放视频
    while(loopFlag)
    {

        if( threadID == MainWindow::mainChnId)
        {
            decoder->Init(threadID, loopCount);
            decoder->Play();
            loopCount++; //循环播放计数.
            if(loopCount >= MainWindow::mainUrl.size())
            {
               loopCount = 0;
            }
        }
        else
        {
             decoder->Init(threadID);
             decoder->Play();
        }

   }
    qDebug() << "线程结束 :" << this->currentThreadId();
    return;
}

void VideoThread::closeThread()
{
    loopFlag = false;
    emit stopDecode(); //停止decodre.
}

VideoThread::~VideoThread()
{

}
