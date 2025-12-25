#include "MainWindow.h"
#include <QApplication>
#include <iostream>
#include "tool.h"
#include "unify/asynctaskrunner.hpp"

Q_DECLARE_METATYPE(unify::EM_TASK_STATE)

void registerMetaType()
{
    qRegisterMetaType<unify::EM_TASK_STATE>("unify::EM_TASK_STATE");
}

void printUsage()
{
    std::cout<<"Usage:"<<std::endl;
    std::cout<<"\t ./StegoTool -c <file1.png,file2.png...>       \t\t 编译"<<std::endl;
    std::cout<<"\t ./StegoTool -w <src.png> <text> [output.png]\t\t 写入文本"<<std::endl;
    std::cout<<"\t ./StegoTool -r <src.png>                    \t\t 读取文本"<<std::endl;
}

int main(int argc, char *argv[]) {
    if(argc == 1)
    {
        QApplication a(argc, argv);
        registerMetaType();
        MainWindow w;
        w.resize(800, 600);
        w.show();

        return a.exec();
    }
    else if(argc>2)
    {
        std::string type = argv[1];
        if(type == "-w")
        {
            if(argc>3)
            {
                std::string src = argv[2];
                std::string text = argv[3];
                std::string tar = "";
                if(argc>4)
                {
                    tar=argv[4];
                }
                embedText(QString::fromStdString(src),QString::fromStdString(tar),QString::fromStdString(text),1);
                return 0;
            }
        }
        else if(type=="-r")
        {
            std::string src = argv[2];
            QString text;
            auto succ = extractText(QString::fromStdString(src),text,1);
            if(succ && !text.isEmpty())
            {
                std::cout<<"==========读取到隐藏文本=========="<<std::endl;
                std::cout<<text.toStdString()<<std::endl;
                return 0;
            }
        }
        else if(type=="-c")
        {
            QCoreApplication a(argc, argv);
            // 解析命令行参数：提取 -c 后的所有内容
            QStringList args = a.arguments();
            int cIndex = args.indexOf("-c");

            // 提取 -c 之后的参数（用户传递给 g++ 的原生参数）
            QStringList gppArgs = args.mid(cIndex + 1);
            if (!gppArgs.isEmpty()) {
                // 核心：调用 g++ 包装函数，直接返回退出码（和 g++ 行为完全一致）
                return executeGcc(gppArgs);
            }
        }
    }
    printUsage();
    return 0;
}

