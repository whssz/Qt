#include "mainwindow.h"
#include "ui_mainwindow.h"



//静态成员,类外声明并初始化.
std::vector<std::string> MainWindow::subUrl;
std::vector<std::string> MainWindow::mainUrl;
uint32_t MainWindow::mainChnId;
//全局变量,默认初始化为0.
QSemaphore sem[31];

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //初始化子通道VideoUrl
    subVideoCount = initSubUrl("D:/6.QT/zhuhai/sub_chn_video_list");
    mainVideoCount = initMainUrl("D:/6.QT/zhuhai/main_chn_video_list");
    qDebug() << "sub_video_Count" << subVideoCount << "  main_video_count" << mainVideoCount;
    MainWindow::mainChnId = subVideoCount; //主通道号.

    //绑定按键-->线程启动和停止
    connect(ui->btn1, &QPushButton::clicked, this, &MainWindow::threadsCreate);
    connect(ui->btn2, &QPushButton::clicked, this, &MainWindow::threadsClose);

    //初始化json关键字列表
    firstClassType << "Vehicles" << "Pedestrains"<< "Bikes";
    QStringList carAttr({"Brand", "Color", "Type", "Plate"});
    QStringList peopleAttr({"Sex", "Age",  "Knapsack", "Hat", "Hair",
                            "UpperType", "UpperColor",
                            "BottomType","BottomColor"});
    QStringList bikeAttr({"Sex", "Age",  "Knapsack", "Hat", "Hair",
                          "UpperType", "UpperColor",
                          "BottomType","BottomColor"});

    //QStringList statClass({"轿车", "商务车", "越野车", "轻客", "大型客车", "行人", "自行车"});
    this->classToAttr.insert(std::pair<QString, QStringList>(firstClassType[0], carAttr));
    this->classToAttr.insert(std::pair<QString, QStringList>(firstClassType[1], peopleAttr));
    this->classToAttr.insert(std::pair<QString, QStringList>(firstClassType[2], peopleAttr));
    this->classToRect.insert(std::pair<QString, QString>(firstClassType[0], "Car"));
    this->classToRect.insert(std::pair<QString, QString>(firstClassType[1], "Body"));
    this->classToRect.insert(std::pair<QString, QString>(firstClassType[2], "Body"));


    //启动定时器,用于刷新显示.25fps.
    QTimer *timer = new QTimer(this);
    timer->setTimerType(Qt::TimerType::PreciseTimer);  //设置为高精度定时器,默认为CoarseTimer(进入到定时器响应函数的时间将有n ms的误差.)
    connect(timer, &QTimer::timeout, this, &MainWindow::disPlay);
    timer->start(40); //1000 / 25 = 40
}

MainWindow::~MainWindow()
{
    delete ui;
}


/******************************线程************************/
//1.创建,启动线程
void MainWindow::threadsCreate()
{
    threadArray.clear();
    for(uint32_t i = 0; i < subVideoCount+1;  ++i)
    {
        //1.new 线程
        threadArray.push_back(new VideoThread(this)); //匿名对象,并将地址拷贝到threadArray中.

        //2.设置线程id
        threadArray[i]->setThreadId(i);

        //3.绑定图片接收信号,此处使用Qt::DirectConnection连接方式,防止信号发送和接收在不同线程所引起的拷贝.
        connect(threadArray[i]->decoder, &QFFmpeg::chnGetImage, this, &MainWindow::imgProcess, Qt::ConnectionType(Qt::DirectConnection));

        //4.启动线程
        threadArray[i]->start();
    }
}

//2.关闭线程
void MainWindow::threadsClose()
{
    /*关闭多线程*/
    for(uint8_t i = 0; i < subVideoCount + 1;  ++i)
    {
        //1.启动线程
        threadArray[i]->closeThread();
        threadArray[i]->wait(); //等待子线程从run()中返回.
    }
}


/******************************json信息************************/
//从json获取信息
bool MainWindow::getInfoFromJson(uint32_t videoId, uint32_t frameId)
{
    //1.清空前一帧的objs.
    this->objs.clear();

    //2.读取json文件,根据frameId读取对应的json文件.需要修改为循环读多个视频.
    QString filePath = QString(":/a%1/a%1_%2.json").arg(videoId + 1).arg(frameId);
    QFile fileIn(filePath);
    //qDebug() << frameId;
    fileIn.open(QIODevice::ReadOnly);  
    if(!fileIn.isOpen()) //如果没有此文件,则直接播放图像.
    {
        qDebug() << filePath << " open fail, skip read frame info!";
        return false;
    }
    QByteArray data = fileIn.readAll();
    fileIn.close();

    //2.解析json文件.
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

    //3.获取json信息
    if(jsonDoc.isObject()) //整个json文件即为1个object
    {
        //获取总json对象
        QJsonObject jsonObj = jsonDoc.object();

        //判断这一帧json数据是否有效.
        QJsonValue msg = jsonObj.value("Message");
        if(msg.toString() == "ok")
        {
            //results为json数组.
            QJsonValue results =  jsonObj.value("ImageResults");
            if(results.isArray())
            {
                QJsonArray resArray = results.toArray();
                int resultCount = resArray.size();
                //qDebug() << "result count is : " << resultCount;

                QJsonObject obj;
                //遍历results,分别遍历 Vehicles/Pedestrains/bike
                for(int i = 0; i < resultCount; ++i)
                {
                    obj = resArray[i].toObject();
                    QJsonArray firstObjArr;
                    int mapSize = static_cast<int>(classToAttr.size());
                    for(int i = 0; i < mapSize; ++i)
                    {
                        if(getFistInfoFromJson(firstClassType[i], obj, firstObjArr))
                        {
                            int arrSize = firstObjArr.size();
                            //qDebug() << firstClassType[i] << " count is : " << arrSize;

                            QJsonObject firstObj;
                            for(int j = 0; j < arrSize; ++j) //每个元素.
                            {
                                firstObj = firstObjArr[j].toObject();
                                getSecInfoFromJson(classToRect.at(firstClassType[i]), classToAttr.at(firstClassType[i]), firstObj); //读取信息.
                            }

                        }
                    }

                }

            }
            else
            {
                qDebug() << "results is not array!";
            }

        }
        else
        {
            qDebug() << "message is invalid!";
        }
    }

    //打印结果信息.
    /*
    for(auto obj : this->objs)
    {
        for(auto attr : obj.attrs)
        {
            qDebug() << attr.first << " :" << attr.second;
        }

        QRect rectArr = obj.rect;
        QString tmpStr = QString("x: %1, y: %2, w: %3, h: %4").arg(rectArr.x()).arg(rectArr.y()).arg(rectArr.width()).arg(rectArr.height());
        qDebug() << tmpStr;
    }
    */
    return true;
}


//1级结构化信息,返回json数组
bool MainWindow::getFistInfoFromJson(QString keyword, QJsonObject &obj, QJsonArray &firstObjArr)
{
    QJsonValue value ;
    if(obj.contains(keyword))
    {
         value = obj.value(keyword);
         //1级结构化的值为数组.
         if(value.isArray())
         {
             firstObjArr = value.toArray(); //获取array.
         }
         else
         {
             return false;
         }
    }
    else
    {
        return false;
    }

    return true;
}

//2接结构化信息
bool MainWindow::getSecInfoFromJson(QString rectKeyWord, QStringList attrKeywordList, QJsonObject &obj)
{

    QJsonObject subObj;
    QJsonObject sub2Obj;
    QJsonObject sub3Obj;
    QJsonValue value;
    ObjInfo info;

    //1.自行车要单独读取Persons信息,递归调用本函数,Persons和Pedestrains属性列表相同.
    if(obj.contains("Persons"))
    {

        QJsonArray arr = obj.value("Persons").toArray();
        int size = arr.size();
        for(int i = 0; i < size; ++i)
        {
            subObj = arr[i].toObject();
            getSecInfoFromJson(rectKeyWord, attrKeywordList, subObj);
        }
        return true;
    }

    //2.读取rect信息
    if(getSubObj( "Detect",obj, subObj))
    {
        if(getSubObj(rectKeyWord, subObj, sub2Obj))
        {
            //判断阈值
            /*
            int score = sub2Obj.value("Score").toInt();
            if(score < SCORE_THRESHOLD)
            {
                return false;
            }
            */
            QJsonArray rectArr = sub2Obj.value("Rect").toArray();           
            QRect rect(rectArr[0].toInt(),rectArr[1].toInt(), rectArr[2].toInt(), rectArr[3].toInt());
            //QString tmpStr = QString("x: %1, y: %2, w: %3, h: %4")
                            // .arg(rectArr[0].toInt()).arg(rectArr[1].toInt()).arg(rectArr[2].toInt()).arg(rectArr[3].toInt());
            //qDebug() << tmpStr;
            info.rect = rect;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    //3.读取属性信息
    if(getSubObj("Recognize", obj, subObj))
    {
        //遍历所有属性.
        for(int i = 0; i < attrKeywordList.size(); ++i)
        {
            if(getSubObj(attrKeywordList[i], subObj, sub2Obj))
            {
               //车牌的属性关键字为Plate而非TopList.
               if(attrKeywordList[i] == "Plate")
               {
                   if(sub2Obj.contains("Licence"))
                   {
                       info.attrs.push_back(std::pair<QString, QString>(attrKeywordList[i], sub2Obj.value("Licence").toString()));
                   }
               }
               else
               {
                   if(sub2Obj.contains("TopList"))
                   {
                       sub3Obj = sub2Obj.value("TopList")[0].toObject();

                       /*
                       int score = sub3Obj.value("Score").toInt(); //判断该属性是否大于阈值.
                       if(score < SCORE_THRESHOLD)
                       {
                           continue;
                       }
                       */
                       info.attrs.push_back(std::pair<QString, QString>(attrKeywordList[i], sub3Obj.value("Name").toString()));
                   }
                   //如果不包含,直接下一次循环.

               }
            }
            else
            {
                continue;
            }
        }
    }
    else
    {
        //JSON_LOG;
        return false;
    }

    //存储到objs中.
    this->objs.push_back(info);

    return true;

}


bool MainWindow::getSubObj(QString keyWord, QJsonObject &obj, QJsonObject &subObj)
{
     QJsonValue value;
    //1.判断是否含该关键字
    if(obj.contains(keyWord))
    {
         value = obj.value(keyWord);
         //2.判断该关键字对应的值是否为jsonobj.
         if(value.isObject())
         {
               subObj = value.toObject();

         }
         else
         {
             return false;
         }
   }
   else
   {
       return false;
   }

   return true;
}



/**********************************视频显示**************************/
//1.单路视频显示耗时: 0.001ms,32路同时刷新并不影响输出帧率的精确控制.
 void MainWindow::disPlay()
 {
     AVPicture tempPic;
     int width, height;
     unsigned int threadId = 0;
     //如果队列非空,待修改.
     for(auto thread : threadArray)
     {
       if(thread->decoder->bufQueue.deQueue(tempPic))
       {

           //1.转换为img类型
           QTime time = QTime::currentTime();
           qDebug() << time.toString("hh:mm:ss.zzz ");
           width = thread->decoder->VideoWidth();
           height = thread->decoder->VideoHeight();
           QImage image(tempPic.data[0],width,height,QImage::Format_RGB888);

           //2.显示图像,需要修改为特定label.
           threadId = thread->getThreadId();
           if(threadId == MainWindow::mainChnId)
           {
                 ui->mainLabel->setPixmap(QPixmap::fromImage(image));
           }
           else if(threadId == 0)
           {
                ui->label1->setPixmap(QPixmap::fromImage(image));
           }
           else
           {
               ui->label2->setPixmap(QPixmap::fromImage(image));
           }

           time = QTime::currentTime();
           qDebug() << time.toString("hh:mm:ss.zzz ");
       }
     }
 }


 void MainWindow::imgProcess(QImage &img, uint32_t theadId, uint32_t videoId, uint32_t frameId)
 {
     if(theadId == MainWindow::mainChnId)
     {
         //1.处理图像
         getInfoFromJson(videoId, frameId);
         drawInfo(img);
         drawRect();
     }

     //2.图像入队列.
     threadArray[theadId]->decoder->bufQueue.enQueue();
     sem[theadId].release(1); //释放信号量

     //3.存入本地
 //        static int i;
 //        i++;
 //        QString str = QString("D:\\6.QT\\zhuhai\\json_file\\output\\img%1.jpg").arg(i);
 //        img.save(str);
 }


 //绘制信息到图像
 void MainWindow::drawInfo(QImage &img)
 {
     //1.设置画家属性
     QPainter pointer(&img);
     QPen pen(QColor(0,255,0));
     pointer.setPen(pen);
     static int width = img.width();
     static int height = img.height();
     static float wScale = width / static_cast<float>(ORG_WIDTH); //画框缩放比例.
     static float hScale = height / static_cast<float>(ORG_HEIGHT);
     //qDebug() << width << height << wScale << hScale;

     //2.遍历所有objInfo对象,画框
     QRect rect;
     auto end = objs.end();
     for(auto iter = objs.begin(); iter != end; ++iter)
     {

        //根据视频size对框进行缩放.
        rect.setX(static_cast<int>(iter->rect.x() * wScale));
        rect.setY(static_cast<int>(iter->rect.y() * hScale));
        rect.setWidth(static_cast<int>(iter->rect.width() * wScale));
        rect.setHeight(static_cast<int>(iter->rect.height() * hScale));
        pointer.drawRect(rect);
        //QString tmpStr = QString("draw info: x: %1, y: %2, w: %3, h: %4")
                         //.arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
        //qDebug() << tmpStr;

        QString attrStr;
        for(auto attr : iter->attrs)
        {
           attrStr += attr.first + ":" +attr.second +"\n";
        }
        pointer.drawText(QRect(QPoint(rect.x(), rect.y()),QSize(300, 300)), Qt::AlignLeft, attrStr);
     }
 }


 //在单独的widget中显示框的信息.
 void MainWindow::drawRect()
 {
     //计算缩放比例
     static float wScale = ui->label3->width() / static_cast<float>(ORG_WIDTH); //画框缩放比例.
     static float hScale = ui->label3->height() / static_cast<float>(ORG_HEIGHT);

    //遍历所有objs,绘制图像.
    QRect rect;
    QPixmap img(QSize( ui->label3->width(), ui->label3->height())); //进行初始化,分配内存.
    img.fill(Qt::black);
    QPainter painter(&img);
    QPen pen(QColor(0,255,0));
    painter.setPen(pen);
    for(auto obj : objs)
    {
        rect.setX(static_cast<int>(obj.rect.x() * wScale));
        rect.setY(static_cast<int>(obj.rect.y() * hScale));
        rect.setWidth(static_cast<int>(obj.rect.width() * wScale));
        rect.setHeight(static_cast<int>(obj.rect.height() * hScale));
        painter.drawRect(rect);
    }
    //绘制.
    ui->label3->setPixmap(img);
 }


 uint32_t MainWindow::initSubUrl(const std::string filename)
 {
     std::string lineBuffer;
     std::ifstream fin(filename);
     while(std::getline(fin, lineBuffer))
     {
         subUrl.push_back(lineBuffer);
     }

     return (uint32_t)subUrl.size();
 }

  uint32_t MainWindow::initMainUrl(const std::string filename)
  {
      std::string lineBuffer;
      std::ifstream fin(filename);
      while(std::getline(fin, lineBuffer))
      {
          mainUrl.push_back(lineBuffer);
      }

      return (uint32_t)mainUrl.size();
  }
