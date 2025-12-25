#include "fullscreenpopup.h"
#include <QApplication>
#include <QDateTime>
#include <cstring>
#include "screensaverwindow.h"
#include "cornerpopup.h"
#include "commandhandler.h"

int main(int argc, char *argv[])
{
    QApplication   app(argc, argv);
    app.setApplicationName("reminder");
    app.setApplicationVersion("1.0");
    // 用当前时间的毫秒数初始化随机种子（每次启动值不同）
    qsrand(static_cast<uint>(QDateTime::currentMSecsSinceEpoch()));

    CommandHandler handler(app);
    QStringList args;
    for (int i = 0; i < argc; ++i)
    {
        args << argv[i];
    }
    handler.processArguments(args);
    return handler.execute();
}
