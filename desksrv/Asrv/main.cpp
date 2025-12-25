#include <QApplication>
#include <QTextCodec>

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
    ftruncate(fdLockFile, 0);
    sprintf(szPid, "%ld", (long)getpid());
    write(fdLockFile, szPid, strlen(szPid) + 1);

    return false;
}

int main(int argc, char *argv[])
{
    if (isAlreadyRunning("Asrv"))
    {
        return 1;
    }
    QApplication a(argc, argv);
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    // ========== 新增：版本更新检查 ==========
    VersionManager versionMgr;
    // 读取版本配置文件
    QJsonObject versionInfo = versionMgr.readVersionConfig("version.json");
    // ========== 新增：打印版本信息 ==========
    QString currentVersion = versionInfo["currentVersion"].toString();
    QString latestVersion  = versionInfo["latestVersion"].toString();
    // 打印到控制台（qDebug）
    qDebug() << "[版本检查] 当前程序版本：" << currentVersion;
    qDebug() << "[版本检查] 最新配置版本：" << latestVersion;

    UpdateDialog* updateDlg = new UpdateDialog(versionInfo);
    updateDlg->setAttribute(Qt::WA_DeleteOnClose); // 关闭弹窗时自动释放内存
    a.setQuitOnLastWindowClosed(false);
    updateDlg->setModal(false);
    updateDlg->show();

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
