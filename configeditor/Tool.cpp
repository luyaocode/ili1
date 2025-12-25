#include "Tool.h"
#include <QApplication>

QIcon getIcon(QStyle::StandardPixmap ePixMap)
{
    return QApplication::style()->standardIcon(ePixMap);
}
