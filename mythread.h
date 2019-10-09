#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QThread>
#include <QImage>
#include "qffmpeg.h"
#include <stdint.h>

class VideoThread : public QThread
{
    Q_OBJECT

public:
    VideoThread(QObject *parent = nullptr);
    ~VideoThread();
    void closeThread();
    void decodeVideo();
    uint32_t getThreadId() const {return threadId;}
    void setThreadId(uint32_t id)  {threadId = id;}
    QFFmpeg *decoder;
    unsigned int loopCount;

signals:
    void stopDecode();

protected:
    virtual void run();

private:
    bool loopFlag = true;
    unsigned int threadId;
};



#endif // MYTHREAD_H
