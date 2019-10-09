#ifndef QFFMPEG_H
#define QFFMPEG_H

//必须加以下内容,否则编译不能通过,为了兼容C和C99标准
#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif

//引入ffmpeg头文件
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
}

#include <QObject>
#include <QMutex>
#include <QImage>
#include <stdint.h>
#include <QTime>
#include "myqueue.h"


class QFFmpeg : public QObject
{
    Q_OBJECT  //允许使用信号与槽机制.
public:
    explicit QFFmpeg(QObject *parent = nullptr);
    ~QFFmpeg();

    bool Init(unsigned int threadId);
    bool Init(unsigned int threadId, unsigned int loopCount);
    void Play();
    void Start();
    void Stop();
    bool CheckTimeOut(uint64_t ul_current_time);
    bool isRunning(){return isRuning;}
    void SetUrl(QString url){this->url=url;}
    QString Url()const{return url;}
    int VideoWidth()const{return videoWidth;}
    int VideoHeight()const{return videoHeight;}
    void SetDecoderId(uint32_t id){this->decoderId=id;}
    uint32_t GetDecoderId()const {return decoderId;}
    uint32_t GetVideoId()const {return videoId;}
    myQueue bufQueue;

private:
    QMutex mutex;
    AVPicture  pAVPicture;
    AVFormatContext *pAVFormatContext;   //格式描述
    AVCodecContext *pAVCodecContext;
    AVFrame *pAVFrame;                   //解码后的数据
    SwsContext * pSwsContext;
    AVPacket pAVPacket;                  //待解码的数据
    AVDictionary* pAVoptions;
    QString url;
    int videoWidth;
    int videoHeight;
    int videoStreamIndex;
    bool isRuning;
    uint64_t last_receive_frame_time_ = 0;
    uint8_t max_receive_time_out_ = 3;
    uint32_t decoderId; //对应线程编号.
    unsigned int videoId = 0; //用于主通道循环播放不同视频.

signals:
    void chnGetImage(QImage &image, unsigned int decoderId, unsigned int videoId = 0, unsigned int framdId = 0);


};

#endif // QFFMPEG_H
