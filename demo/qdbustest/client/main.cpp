#include "client.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    DBusClient client;
    client.syncCall();

    return app.exec();
//    return 0;
}
