#ifndef MYQUEUE_H
#define MYQUEUE_H

#include <QObject>
#include <QDebug>

extern "C"
{
#include <libavformat/avformat.h>
}

class myQueue : public QObject
{
    Q_OBJECT
    friend class QFFmpeg;
public:
    explicit myQueue(QObject *parent = nullptr, int bufCount = MAXSIZE);
    ~myQueue();

    bool enQueue(); //数据类型
    bool deQueue(AVPicture &pRecvData);
    AVPicture& getTailItem();  //获取尾指针buffer.
    //void deTraverse(); //遍历队列中存储的数据元素
    int getSize();    //获取队列中的数据量
    bool isFull();
    bool isEmpty();
    //int getLength() {return length;}

    enum {MAXSIZE = 5};
    AVPicture *pArr; //队列首地址.
    int head,tail;
    int length;//队列的实际长度

private:


signals:

public slots:
};

#endif // MYQUEUE_H

