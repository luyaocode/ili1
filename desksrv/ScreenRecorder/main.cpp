#include <QApplication>
#include "commandhandler.h"
#include "keyblocker.h"

int main(int argc, char *argv[])
{
    QApplication   app(argc, argv);
    app.setApplicationName("ScreenRecorder");
    app.setApplicationVersion("1.0");

    qRegisterMetaType<KeyWithModifier>("KeyWithModifier");
    qRegisterMetaType<std::vector<KeyWithModifier>>("std::vector<KeyWithModifier>");

    CommandHandler handler(app);
    QStringList args;
    for (int i = 0; i < argc; ++i)
    {
        args << argv[i];
    }
    handler.processArguments(args);
    return handler.execute();
}
