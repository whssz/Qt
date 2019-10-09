#include "qffmpeg.h"
#include <QDebug>
#include <utility>
#include <QCoreApplication>
#include "mainwindow.h"
#include <QTime>

#define FFMPEG_VERSION_3_1 AV_VERSION_INT(57, 40, 100)

extern QSemaphore sem[31];

static uint64_t GetTickCount() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static int InterruptCallBack(void* ctx) {
  QFFmpeg* pvideounpack = reinterpret_cast<QFFmpeg*>(ctx);
  if (pvideounpack->CheckTimeOut(GetTickCount())) {
    return 1;
  }
  return 0;
}

bool QFFmpeg::CheckTimeOut(uint64_t ul_current_time) {
  if ((ul_current_time - last_receive_frame_time_) / 1000 > max_receive_time_out_) {
    return true;
  }
  return false;
}

QFFmpeg::QFFmpeg(QObject *parent) :
    QObject(parent),  pAVFormatContext(nullptr), //初始化列表
    pAVCodecContext(nullptr), pAVFrame(nullptr),
    pSwsContext(nullptr)
{
    videoStreamIndex=-1;
    pAVoptions = nullptr;

    av_register_all();//注册库中所有可用的文件格式和解码器
    avformat_network_init();//初始化网络流格式,使用RTSP网络流时必须先执行
    last_receive_frame_time_ = GetTickCount(); //
    pAVFrame=av_frame_alloc(); //this only allocates the AVFrame itself, not the data buffers

}

QFFmpeg::~QFFmpeg()
{

    if (pAVFormatContext) {
        avformat_free_context(pAVFormatContext);
    }
    if (pAVFrame) {
        av_frame_free(&pAVFrame);
    }
    if (pSwsContext) {
        sws_freeContext(pSwsContext);
    }
    if(pAVoptions){
        av_dict_free(&pAVoptions);
    }
}

bool QFFmpeg::Init(unsigned int threadId, unsigned int loopCount)
{
    av_dict_set(&pAVoptions, "buffer_size", "1024000", 0); //102400
    av_dict_set(&pAVoptions, "max_delay", "500000", 0);
    av_dict_set(&pAVoptions, "stimeout", "20000000", 0);  //设置超时断开连接时间

    //设置与线程编号对应的decoerId
    SetDecoderId(threadId);

    //初始化url  
    url = QString::fromStdString(MainWindow::mainUrl[loopCount]); //待修改,循环播放.
    videoId = loopCount;

    //分配pAVFormatContext结构内存,循环视频播放,需要重新初始化pAVFormatContext
    pAVFormatContext = avformat_alloc_context();//申请一个AVFormatContext结构的内存,并进行简单初始化
    AVIOInterruptCB intrpt_callback = {InterruptCallBack, this};
    pAVFormatContext->interrupt_callback = intrpt_callback;

    //打开视频流
    int result=avformat_open_input(&pAVFormatContext, url.toStdString().c_str(),NULL,&pAVoptions);
    if (result<0){
        qDebug()<<url<<endl;
        qDebug()<<"open media url failed";
        return false;
    }

    //获取视频流信息
    result=avformat_find_stream_info(pAVFormatContext,NULL);
    if (result<0){
        qDebug()<<"fetch media info failed";
        return false;
    }

    printf("--------------- File Information ----------------\n");
    //此函数打印输入或输出的详细信息

    av_dump_format(pAVFormatContext, 0, url.toStdString().c_str(), 0);
    printf("-------------------------------------------------\n");

    //获取视频流索引
    videoStreamIndex = -1;
    for (uint i = 0; i < pAVFormatContext->nb_streams; i++) {
        if (pAVFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex==-1){
        qDebug()<<"fetch media index failed"<<endl;
        return false;
    }

    //获取视频流的分辨率大小
    pAVCodecContext = pAVFormatContext->streams[videoStreamIndex]->codec;
    videoWidth=pAVCodecContext->width;
    videoHeight=pAVCodecContext->height;

    //为sws_scale转换后rgb图像数据分配动态内存.
    //avpicture_alloc(&pAVPicture,AV_PIX_FMT_RGB24,videoWidth,videoHeight);
    //初始化imgqueue,为队中元素的图像数据分配动态内存.
    for(int i = 0; i < bufQueue.length; ++i)
          avpicture_alloc(&bufQueue.pArr[i],AV_PIX_FMT_RGB24,videoWidth,videoHeight);


    AVCodec *pAVCodec;
    //获取视频流解码器
    pAVCodec = avcodec_find_decoder(pAVCodecContext->codec_id);
    pSwsContext = sws_getContext(videoWidth,videoHeight,AV_PIX_FMT_YUV420P,videoWidth,videoHeight,AV_PIX_FMT_RGB24,SWS_BICUBIC,0,0,0);

    //打开对应解码器
    result=avcodec_open2(pAVCodecContext,pAVCodec,NULL);
    if (result<0){
        qDebug()<<"open codec failed"<<endl;
        return false;
    }

    qDebug()<<"video init success";

    isRuning= true;
    return true;
}



bool QFFmpeg::Init(unsigned int threadId)
{
    av_dict_set(&pAVoptions, "buffer_size", "1024000", 0); //102400
    av_dict_set(&pAVoptions, "max_delay", "500000", 0);
    av_dict_set(&pAVoptions, "stimeout", "20000000", 0);  //设置超时断开连接时间

    //设置与线程编号对应的decoerId
    SetDecoderId(threadId);

    //初始化url
    url = QString::fromStdString(MainWindow::subUrl[decoderId]);

    //分配pAVFormatContext结构内存,循环视频播放,需要重新初始化pAVFormatContext
    pAVFormatContext = avformat_alloc_context();//申请一个AVFormatContext结构的内存,并进行简单初始化
    AVIOInterruptCB intrpt_callback = {InterruptCallBack, this};
    pAVFormatContext->interrupt_callback = intrpt_callback;

    //打开视频流
    int result=avformat_open_input(&pAVFormatContext, url.toStdString().c_str(),NULL,&pAVoptions);
    if (result<0){
        qDebug()<<url<<endl;
        qDebug()<<"open media url failed";
        return false;
    }

    //获取视频流信息
    result=avformat_find_stream_info(pAVFormatContext,NULL);
    if (result<0){
        qDebug()<<"fetch media info failed";
        return false;
    }

    printf("--------------- File Information ----------------\n");
    //此函数打印输入或输出的详细信息

    av_dump_format(pAVFormatContext, 0, url.toStdString().c_str(), 0);
    printf("-------------------------------------------------\n");

    //获取视频流索引
    videoStreamIndex = -1;
    for (uint i = 0; i < pAVFormatContext->nb_streams; i++) {
        if (pAVFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex==-1){
        qDebug()<<"fetch media index failed"<<endl;
        return false;
    }

    //获取视频流的分辨率大小
    pAVCodecContext = pAVFormatContext->streams[videoStreamIndex]->codec;
    videoWidth=pAVCodecContext->width;
    videoHeight=pAVCodecContext->height;

    //为sws_scale转换后rgb图像数据分配动态内存.
    //avpicture_alloc(&pAVPicture,AV_PIX_FMT_RGB24,videoWidth,videoHeight);
    //初始化imgqueue,为队中元素的图像数据分配动态内存.
    for(int i = 0; i < bufQueue.length; ++i)
          avpicture_alloc(&bufQueue.pArr[i],AV_PIX_FMT_RGB24,videoWidth,videoHeight);


    AVCodec *pAVCodec;
    //获取视频流解码器
    pAVCodec = avcodec_find_decoder(pAVCodecContext->codec_id);
    pSwsContext = sws_getContext(videoWidth,videoHeight,AV_PIX_FMT_YUV420P,videoWidth,videoHeight,AV_PIX_FMT_RGB24,SWS_BICUBIC,0,0,0);

    //打开对应解码器
    result=avcodec_open2(pAVCodecContext,pAVCodec,NULL);
    if (result<0){
        qDebug()<<"open codec failed"<<endl;
        return false;
    }

    qDebug()<<"video init success";

    isRuning= true;
    return true;
}



void QFFmpeg::Stop()
{
    isRuning = false;
    qDebug()<<"video stop";

}

void Delay(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < dieTime )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100); //最大等待100ms.
}


void QFFmpeg::Play()
{
    //逐帧读取视频
    isRuning= true;
    int frameFinished=0;    
    unsigned int frameId = 0;
    unsigned int videoId = GetVideoId();
    unsigned int chnId = GetDecoderId();
    int sleepCount = 0;
    qRegisterMetaType<QImage>("QImage&"); //注册QImage&的引用类型.
    while (isRuning)
    {
        last_receive_frame_time_ = GetTickCount();
        if (av_read_frame(pAVFormatContext, &pAVPacket) >= 0)//解封装
        {
            if(pAVPacket.stream_index==videoStreamIndex)
            {
                avcodec_decode_video2(pAVCodecContext, pAVFrame, &frameFinished, &pAVPacket); //解码原始数据.
                if (frameFinished)
                {
                    //mutex.lock(); //互斥锁
                    //等待队空
                    sleepCount = 0;
                    while(bufQueue.isFull() && (sleepCount < 100)) //500ms
                    {
                       //队满
                       sleepCount++;
                       QThread::msleep(5);
                    }

                    AVPicture tempPic = bufQueue.getTailItem(); //获取1个buffer.
                    sws_scale(pSwsContext,(const uint8_t* const *)pAVFrame->data,pAVFrame->linesize,0,videoHeight,tempPic.data,tempPic.linesize);

                    //本地生成image
                    QImage image(tempPic.data[0],videoWidth,videoHeight,QImage::Format_RGB888);


                    //显示图像
                    emit chnGetImage(image, chnId, videoId, frameId);

                    //等待获取信号量.
                    sem[decoderId].acquire(1);
                    //mutex.unlock();
                    //qDebug() << "ffmepg frameId:" << frameId;
                    frameId++;  //解码计数
                }
                else
                {
                    qDebug() << "frameid :" << frameId << "解码失败!";
                }
            }
        }
        else
        {
            isRuning = false;
        }
#if LIBAVFORMAT_VERSION_INT >= FFMPEG_VERSION_3_1
        av_packet_unref(&pAVPacket);  //更新pkt,每1帧都需要更新.
#else
        av_free_packet(&pAVPacket);
#endif
    }

    //释放pAVFormatContext.
    if (pAVFormatContext)
        avformat_free_context(pAVFormatContext);

    //释放queue缓存.
    for(int i = 0; i < bufQueue.length; ++i)
          avpicture_free(&bufQueue.pArr[i]);
}






