#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <stdint.h>  //标准头文件.
#include <QPushButton>
#include <QVBoxLayout>
#include <QImage>
#include <QStringList>
#include <vector>
#include <QPushButton>
#include <QFile>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QJsonArray>
#include <utility>
#include <QPainter>
#include <QPixmap>
#include <QPaintDevice>
#include <QRect>
#include <QPaintDevice>
#include <QLabel>
#include <QTime>
#include <QTimer>
#include <string>
#include <fstream>
#include <QMetaType>
#include "qffmpeg.h"
#include "objinfo.h"
#include "mythread.h"
#include <QSemaphore>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT  

    enum {ORG_WIDTH = 1280,ORG_HEIGHT = 720};  //主窗口视频源分辨率,用于主窗口画框缩放比例,scale.
    enum { SCORE_THRESHOLD = 80};  //阈值.

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool getInfoFromJson(uint32_t videoId, uint32_t frameId);
    void drawInfo(QImage &img);
    static uint32_t initSubUrl(const std::string filename);
    static uint32_t initMainUrl(const std::string filename);
    uint32_t getSubVideoCount() {return subVideoCount;}
    uint32_t getMainVideoCount() {return mainVideoCount;}
    void drawRect();
    static std::vector<std::string> subUrl;
    static std::vector<std::string> mainUrl;
    static uint32_t mainChnId; //将主通道id排在子通道id后.


public slots:
    void threadsCreate();    //button --> maintain
    void threadsClose();   //button --> maintain
    void imgProcess(QImage &img, uint32_t threadId, uint32_t videoId = 0, uint32_t frameId = 0);
    void disPlay();

private:
    Ui::MainWindow *ui;
    std::vector<VideoThread *> threadArray;

private:
    bool getFistInfoFromJson(QString keyword, QJsonObject &obj, QJsonArray &firstObjArr);
    bool getSecInfoFromJson(QString rectKeyWord, QStringList attrKeywordList, QJsonObject &obj);
    bool getSubObj(QString keyWord, QJsonObject &obj, QJsonObject &subObj);

    std::vector<ObjInfo> objs;  //存放结果类.
    QStringList firstClassType; //1级结构化类名
    std::map<QString, QStringList> classToAttr; //类名 : 属性键
    std::map<QString, QString> classToRect;        //类名 : 框键
    //std::map<QString, uint32_t> classCount;  //类型 : 个数
private:

    uint32_t subVideoCount = 0;
    uint32_t mainVideoCount = 0;

};
#endif // MAINWINDOW_H
