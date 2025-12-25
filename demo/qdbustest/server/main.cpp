#include "service.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    DBusService service;
    service.run();

    return app.exec();
}
