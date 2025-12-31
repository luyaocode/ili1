#include <QApplication>
#include <QTextCodec>
#include <QJsonArray>

#include "commontool/VersionManager.h"
#include "commontool/UpdateDialog.h"
#include "Server.h"
#include "def.h"

extern "C"
{
#include <fcntl.h>
#include <signal.h> /* 信号定义 */
#include <stdarg.h> /* 可变参数定义 */
#include <stdio.h>  /* 标准输入输出定义 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* Unix 标准函数定义 */
}

bool isAlreadyRunning(const std::string &name)
{
    std::string  lock_file  = "/tmp/" + name + ".pid";
    const char  *LOCK_FILE  = lock_file.c_str();
    int          fdLockFile = -1;
    char         szPid[32];
    struct flock fl;

    /* 打开锁文件 */
    fdLockFile = open(LOCK_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fdLockFile < 0)
    {
        fprintf(stderr, "lock file open failed");
        exit(EXIT_FAILURE);
    }

    /* 对整个锁文件加写锁 */
    fl.l_type   = F_WRLCK;   //记录锁类型：独占性写锁
    fl.l_whence = SEEK_SET;  //加锁区域起点:距文件开始处l_start个字节
    fl.l_start  = 0;
    fl.l_len =
        0;  //加锁区域终点:0表示加锁区域自起点开始直至文件最大可能偏移量为止,不管写入多少字节在文件末尾,都属于加锁范围
    if (fcntl(fdLockFile, F_SETLK, &fl) < 0)
    {
        if (EACCES == errno || EAGAIN == errno)  //系统中已有该守护进程的实例在运行
        {
            fprintf(stdout, "%s Already Running...", name.c_str());
            close(fdLockFile);
            return true;
        }

        fprintf(stdout, "%s AlreadyRunning fcntl", name.c_str());

        exit(EXIT_FAILURE);
    }

    /* 清空锁文件,然后将当前守护进程pid写入锁文件 */
    auto ret = ftruncate(fdLockFile, 0);
    (void)ret;
    sprintf(szPid, "%ld", (long)getpid());
    ret = write(fdLockFile, szPid, strlen(szPid) + 1);
    (void)ret;

    return false;
}

// 全局日志处理函数：自动添加线程ID、时间、日志级别
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)
    // 1. 获取当前时间（格式：yyyy-MM-dd hh:mm:ss.zzz）
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    // 2. 获取线程名称ID
    QString       tname;
    unsigned long thread_id = reinterpret_cast<unsigned long>(pthread_self());
    tname                   = QString::number(thread_id);

    // 3. 日志级别字符串
    QString level;
    QString logText;
    switch (type)
    {
        case QtDebugMsg:
            level = "DEBUG";
            break;
        case QtInfoMsg:
        {
            level   = "INFO";
            logText = msg;
            break;
        }
        case QtWarningMsg:
            level = "WARN";
            break;
        case QtCriticalMsg:
            level = "ERROR";
            break;
        case QtFatalMsg:
            level = "FATAL";
            break;
    }
    if (logText.isEmpty())
    {
        // 4. 格式化日志并输出到控制台
        logText = QString("[%1] %2 [%3] %4")
                      .arg(tname)  // 线程ID
                      .arg(time)   // 时间
                      .arg(level)  // 级别
                      .arg(msg);   // 日志内容
    }

    QTextStream(stdout) << logText << endl;

    // 致命错误时终止程序（保持Qt默认行为）
    if (type == QtFatalMsg)
    {
        abort();
    }
}

int main(int argc, char *argv[])
{
    if (isAlreadyRunning("Asrv"))
    {
        return 1;
    }
    QApplication a(argc, argv);
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    // 注册日志监听
    qInstallMessageHandler(customMessageHandler);

    // ========== 新增：版本更新检查 ==========
    VersionManager versionMgr;
    // 读取版本配置文件
    QJsonObject versionInfo = versionMgr.readVersionConfig("version.json");
    // ========== 新增：打印版本信息 ==========
    QString currentVersion = versionInfo["currentVersion"].toString();
    QString latestVersion  = versionInfo["latestVersion"].toString();
    QString updateContent;
    if (versionInfo["updateContent"].isArray())
    {  // 先判断是否为数组
        QJsonArray contentArray = versionInfo["updateContent"].toArray();
        // 方式1：拼接数组为单个字符串（如换行分隔）
        QStringList contentList;
        for (const QJsonValue &value : contentArray)
        {
            contentList.append(value.toString());  // 逐个提取数组中的字符串
        }
        updateContent = contentList.join("\n");  // 用换行拼接成一个字符串
    }
    else
    {
        updateContent = versionInfo["updateContent"].toString();
    }
    // 打印到控制台（qDebug）
    qDebug() << "==============================================================";
    qDebug() << "[版本检查] 当前程序版本：" << currentVersion;
    qDebug() << "[版本检查] 最新配置版本：" << latestVersion;
    qDebug() << "[版本检查] 更新内容：" << updateContent;
    qDebug() << "==============================================================";

    //    UpdateDialog* updateDlg = new UpdateDialog(versionInfo);
    //    updateDlg->setAttribute(Qt::WA_DeleteOnClose); // 关闭弹窗时自动释放内存
    //    a.setQuitOnLastWindowClosed(false);
    //    updateDlg->setModal(false);
    //    updateDlg->show();

    QString ip   = IP_ADDR;
    quint16 port = IP_PORT;
    if (argc > 2)
    {
        ip   = argv[0];
        port = QString(argv[1]).toUInt();
    }
    else if (argc > 1)
    {
        ip = argv[0];
    }
    // 启动 HTTP 服务器
    Server server(ip, port);

    return a.exec();
}
