#ifndef IMGQUEUE_H
#define IMGQUEUE_H

#include <iostream>
#include <QMetaType>
#include <QObject>

extern "C"
{
#include <libavformat/avformat.h>
}


template<class T>
class ImgQueue
{
    friend class QFFmpeg;
public:

    ImgQueue(int items =  static_cast<int>(MaxSize::MAXSIZE));
    ~ImgQueue();
    bool enQueue(T &data); //数据类型
    bool deQueue(T *pRecvData);
    T& getTailItem();  //获取尾指针buffer.
    //void deTraverse(); //遍历队列中存储的数据元素
    int getSize();    //获取队列中的数据量
    bool isFull();
    bool isEmpty();
    //int getLength() {return length;}
private:
    enum class MaxSize{MAXSIZE = 5};
    T *pArr; //队列首地址.
    int head,tail;
    int length;//队列的实际长度
};

#endif // QUEUE_H
