#include <QApplication>
#include "MainWindow.h"
#include <QMetaType>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<QVector<int>>("QVector<int>");
    qRegisterMetaType<QTreeWidgetItem*>("QTreeWidgetItem*");
    MainWindow w;
    w.show();
    return a.exec();
}
