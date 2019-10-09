#include "imgqueue.h"

template<class T>
ImgQueue<T>::ImgQueue(int items)
{
    this->length = items + 1;
    pArr = new T[this->length];
//    for(int i = 0; i < this->length; ++i)
//        avpicture_alloc(&pArr[i],AV_PIX_FMT_RGB24,1280,720);
    head = tail = 0;
}


template<class T>
ImgQueue<T>::~ImgQueue()
{
//    for(int i = 0; i < this->length; ++i)
//        avpicture_free(&pArr[i]);
    delete[] pArr;
}

template<class T>
T& ImgQueue<T>::getTailItem()
{
    return pArr[tail];
}

template<class T>
bool ImgQueue<T>::enQueue(T &data)
{
    if (isFull())
    {
        std::cout<<"队列已满!"<<std::endl;
        return false;
    }
    pArr[tail] = data;
    tail = (tail + 1) % length; //tail指向空元素.
    return true;
}

template<class T>
bool ImgQueue<T>::deQueue(T* pRecvData)
{
    if (isEmpty())
    {
        std::cout << "队列为空！" << std::endl;
        return false;
    }
    if (pRecvData != nullptr)
        *pRecvData = pArr[head];
    head = (head+1) % length;
    return true;
}

template<class T>
int ImgQueue<T>::getSize()
{
    return (tail - head + length) % length;
}

template<class T>
bool ImgQueue<T>::isFull()
{
    return (tail + 1) % length == head;
}

template<class T>
bool ImgQueue<T>::isEmpty()
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
