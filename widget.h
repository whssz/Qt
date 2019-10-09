#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "mythread.h"
#include <QPushButton>
#include <QVBoxLayout>

class Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();
    void createView();

signals:


private slots:
    void openThreadBtnSlot();
    void closeThreadBtnSlot();
    void finishedThreadBtnSlot();

private:
    QVBoxLayout *mainLayout;
    MyThread *threadArray[2];

};


#endif // WIDGET_H
