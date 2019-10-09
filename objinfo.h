#ifndef OBJINFO_H
#define OBJINFO_H

#include <map>
#include <QString>
#include <QRect>
#include <vector>
#include <QStringList>


//存放json结果
class ObjInfo
{
public:
    ObjInfo();

    //1.rect
    QRect rect;
    //2.attribute
    std::vector<std::pair<QString, QString>> attrs;
};

#endif // OBJINFO_H
