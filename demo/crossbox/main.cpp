#include "inc.h"
#include "ballwindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // 设置随机数种子
    qsrand(QTime::currentTime().msecsSinceStartOfDay());

    BallWindow w;
    w.show();

    return a.exec();
}
