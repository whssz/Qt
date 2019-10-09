#include "myqueue.h"

myQueue::myQueue(QObject *parent, int bufCount) : QObject(parent)
{
    this->length = bufCount + 1;
    pArr = new AVPicture[this->length];
    head = tail = 0;
}

myQueue::~myQueue()
{
    delete[] pArr;
}

AVPicture& myQueue::getTailItem()
{
    return pArr[tail];
}

bool myQueue::enQueue()
{
    if (isFull())
    {
        qDebug() <<"队列已满!";
        return false;
    }
    //pArr[tail] = data;
    tail = (tail + 1) % length; //tail指向空元素.
    return true;
}

bool myQueue::deQueue(AVPicture &pRecvData)
{
    if (isEmpty())
    {
         qDebug() <<"队列为空!";
        return false;
    }
    pRecvData = pArr[head];
    head = (head + 1) % length;
    return true;
}
int myQueue::getSize()
{
    return (tail - head + length) % length;
}

bool myQueue::isFull()
{
    return (tail + 1) % length == head;
}

bool myQueue::isEmpty()
{
    return head == tail;
}

/*
template<class T>
void ImgQueue<T>::deTraverse()
{
    if (head == tail)
    {
        std::cout << "队列为空！" << std::endl;
        return;
    }
    for (int i = head; i != tail; i = (i + 1) % length)
        std::cout << pArr[i] << " ";
    std::cout << std::endl;
}
*/
