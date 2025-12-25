#include "tool.h"
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QTextStream>
#include <QTemporaryFile>
#include <QTextCodec>  // 需包含头文件
#include <QImage>
#include <QPainter>
#include <QLinearGradient>
#include <QPainterPath>
#include <QDebug>
#include <iostream>
#include "StegoCore.h"

bool embedText(const QString &strSource, const QString &strTarget, const QString &strText, int bitCount)
{
    QFileInfo fileInfo(strSource);
    if (!fileInfo.exists())
    {
        std::cout << "Failed to find file: " << strSource.toStdString() << std::endl;
        return false;
    }
    QImage originalImage;
    originalImage.load(strSource);
    if (originalImage.isNull())
    {
        std::cout << "Failed to load img: " << strSource.toStdString() << std::endl;
        return false;
    }
    if (strText.isEmpty())
    {
        std::cout << "Text is empty." << std::endl;
        return false;
    }
    QImage targetImage;
    try
    {
        bool success = StegoCore::embedText(originalImage, strText, targetImage, nullptr, nullptr, bitCount);
        if (success)
        {
            if (targetImage.isNull())
            {
                std::cout << "Failed to embed text, target img is Null." << std::endl;
                return false;
            }
            QString strTargetPath = strTarget;
            if (strTarget.isEmpty())
            {
                QString pathWithoutSuffix =
                    QDir::cleanPath(fileInfo.path() + "/" + fileInfo.baseName()  // 目录 + 无后缀文件名
                    );
                QString strFormattedTimestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
                QString suffix                = fileInfo.baseName().isEmpty() ? "png" : fileInfo.suffix();
                strTargetPath                 = pathWithoutSuffix + "_" + strFormattedTimestamp + "." + suffix;
            }
            if (targetImage.save(strTargetPath))
            {
                std::cout << "Success: " << strTargetPath.toStdString() << std::endl;
                return true;
            }
            else
            {
                std::cout << "Failed to save img: " << strTargetPath.toStdString() << std::endl;
                return true;
            }
        }
        else
        {
            std::cout << "Failed to embed text." << std::endl;
            return false;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool extractText(const QString &strSource, QString &text, int bitCount)
{
    QFileInfo fileInfo(strSource);
    if (!fileInfo.exists())
    {
        std::cout << "Failed to find file: " << strSource.toStdString() << std::endl;
        return false;
    }
    QImage originalImage;
    originalImage.load(strSource);
    if (originalImage.isNull())
    {
        std::cout << "Failed to load img: " << strSource.toStdString() << std::endl;
        return false;
    }
    try
    {
        bool succ = StegoCore::extractText(originalImage, text, nullptr, nullptr, bitCount);
        return succ;
    }
    catch (const std::runtime_error &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

// 全局文本流（保持错误提示风格与 g++ 一致）
QTextStream cout(stdout);
QTextStream cerr(stderr);

/**
 * @brief Linux 下通过 which 命令查找 g++ 路径（替代 QProcess::findExecutable）
 * @return QString g++ 完整路径（未找到返回空）
 */
static QString findGccPath()
{
    QProcess whichProcess;
    whichProcess.setProgram("which");
    whichProcess.setArguments({"g++"});  // 执行 which g++
    whichProcess.setProcessChannelMode(QProcess::MergedChannels);

    // 启动 which 命令并等待执行完成
    whichProcess.start();
    if (!whichProcess.waitForStarted() || !whichProcess.waitForFinished(1000))
    {
        return "";  // 启动失败或超时
    }

    // 读取 which 输出（结果是 g++ 路径，末尾带换行符）
    QString gppPath = QString::fromLocal8Bit(whichProcess.readAll()).trimmed();
    // 验证路径是否存在且可执行
    QFileInfo fileInfo(gppPath);
    if (gppPath.isEmpty() || !fileInfo.exists() || !fileInfo.isExecutable())
    {
        return "";
    }

    return gppPath;
}

// 辅助函数：执行默认可执行文件 a.out（关键简化）
int executeAOut()
{
    QDir    dir("a.out");
    QString exePath = dir.absolutePath();  // 固定使用 a.out

    // 检查 a.out 是否存在（编译成功应该生成）
    if (!QFile::exists(exePath))
    {
        cerr << U8("错误：编译成功但未找到可执行文件！") << endl;
        return 127;
    }

    // 检查执行权限（Linux/macOS 必要）
    QFileInfo exeInfo(exePath);
    if (!exeInfo.isExecutable())
    {
        cerr << U8("警告：无执行权限，尝试添加...") << endl;
        QProcess chmodProcess;
        chmodProcess.setProgram("chmod");
        chmodProcess.setArguments({"+x", exePath});
        chmodProcess.start();
        chmodProcess.waitForFinished();
        if (chmodProcess.exitCode() != 0)
        {
            cerr << U8("错误：添加执行权限失败！") << endl;
            return 126;
        }
    }

    // 执行 a.out
    QProcess exeProcess;
    exeProcess.setProcessChannelMode(QProcess::MergedChannels);
    exeProcess.setProgram(exePath);

    cout << U8("start: ") << exeInfo.absoluteFilePath() << endl;
    exeProcess.start();
    if (!exeProcess.waitForStarted())
    {
        cerr << U8("错误：启动程序失败！原因：") << exeProcess.errorString().toLocal8Bit().data() << endl;
        return 1;
    }

    // 实时输出执行日志
    QByteArray buffer;
    while (exeProcess.state() == QProcess::Running)
    {
        if (exeProcess.waitForReadyRead(100))
        {
            buffer = exeProcess.readAll();
            cout << U8(buffer.data());
            cout.flush();
        }
    }

    // 读取剩余输出
    buffer = exeProcess.readAll();
    if (!buffer.isEmpty())
    {
        cout << U8(buffer.data());
        cout.flush();
    }

    // 等待执行完成
    exeProcess.waitForFinished();
    int exitCode = exeProcess.exitCode();
    cout << U8("=== end ===") << endl;

    // 可选：执行后删除 a.out（根据需求决定是否保留）
    if (QFile::remove(exePath))
    {
        //        cout << U8("程序已自动删除") << endl;
    }
    else
    {
        //        cerr << U8("警告：程序删除失败，需手动清理") << endl;
    }

    return exitCode;
}

int executeGcc(const QStringList &gppArgs)
{
    // 步骤 1：查找 g++ 编译器（保持原逻辑）
    QString gppPath = findGccPath();
    if (gppPath.isEmpty())
    {
        cerr << U8("错误：未找到 g++ 编译器！请确保 g++ 已安装并添加到系统 PATH 中。") << endl;
        return 127;
    }

    // 步骤 2：从 gppArgs 中提取所有 .png 文件路径
    QStringList pngFiles;
    for (const QString &arg : gppArgs)
    {
        // 匹配后缀为 .png 的文件（不区分大小写，可选）
        if (arg.endsWith(".png", Qt::CaseInsensitive))
        {
            // 转换为绝对路径，避免相对路径问题
            QDir    dir(arg);
            QString absPath = dir.absolutePath();
            if (QFile::exists(absPath))
            {
                pngFiles << absPath;
            }
            else
            {
                cerr << U8("警告：PNG 文件不存在 - ") << U8(absPath.toLocal8Bit().data()) << endl;
            }
        }
    }

    // 检查是否有有效 PNG 文件
    if (pngFiles.isEmpty())
    {
        cerr << U8("错误：未从参数中找到有效的 .png 文件！") << endl;
        return 2;
    }

    // 步骤 3：处理每个 PNG 文件，提取源代码并生成临时 .cpp 文件
    QStringList tempCppFiles;          // 存储临时文件路径（用于后续清理）
    QStringList newGppArgs = gppArgs;  // 新的编译参数（替换 PNG 为临时 CPP）

    for (const QString &pngFile : pngFiles)
    {
        // 3.1 提取源代码
        QString sourceCode;
        bool    succ = extractText(pngFile, sourceCode);
        if (!succ || sourceCode.isEmpty())
        {
            cerr << U8("错误：从 PNG 文件提取源代码失败 - ") << U8(pngFile.toLocal8Bit().data()) << endl;
            // 清理已生成的临时文件
            for (const QString &tempCpp : tempCppFiles)
            {
                QFile::remove(tempCpp);
            }
            return 3;
        }
        //       cout << sourceCode << endl;
        cout.flush();

        // 3.2 创建 Qt 临时文件（自动命名，后缀 .cpp，关闭后自动删除）
        QTemporaryFile tempFile(QDir::tempPath() + "/tmp_XXXXXX.cpp");
        // 重要：设置为“关闭后不自动删除”，因为 QProcess 需要后续读取
        tempFile.setAutoRemove(false);
        if (!tempFile.open())
        {
            cerr << U8("错误：创建临时 .cpp 文件失败！原因：") << U8(tempFile.errorString().toLocal8Bit().data())
                 << endl;
            for (const QString &tempCpp : tempCppFiles)
            {
                QFile::remove(tempCpp);
            }
            return 4;
        }

        // 3.3 写入提取的源代码
        QTextStream out(&tempFile);
        out.setCodec("UTF-8");
        out << sourceCode;
        tempFile.close();  // 关闭文件，让 g++ 可以读取

        // 3.4 记录临时文件路径（用于替换参数和后续清理）
        QString tempCppPath = tempFile.fileName();
        tempCppFiles << tempCppPath;

        // 3.5 替换新参数中的 PNG 路径为临时 CPP 路径（替换所有匹配项）
        newGppArgs.replaceInStrings(pngFile, tempCppPath);
        QDir    currentDir;  // 默认是当前工作目录
        QString relativePngPath = currentDir.relativeFilePath(pngFile);
        if (relativePngPath != pngFile)
        {
            newGppArgs.replaceInStrings(relativePngPath, tempCppPath);
        }
    }

    // 步骤 4：验证新参数中是否包含临时 CPP 文件（避免替换失败）
    bool hasTempCpp = false;
    for (const QString &arg : newGppArgs)
    {
        if (arg.endsWith(".cpp", Qt::CaseInsensitive))
        {
            hasTempCpp = true;
            break;
        }
    }
    if (!hasTempCpp)
    {
        cerr << U8("错误：替换 PNG 参数为临时 .cpp 文件失败！") << endl;
        for (const QString &tempCpp : tempCppFiles)
        {
            QFile::remove(tempCpp);
        }
        return 5;
    }

    // 步骤 5：执行 g++ 编译（复用原逻辑，使用新参数 newGppArgs）
    QProcess gppProcess;
    gppProcess.setProcessChannelMode(QProcess::MergedChannels);
    gppProcess.setProgram(gppPath);
    // 编译选项
    newGppArgs.prepend("-std=c++11");
    newGppArgs.prepend("-pthread");
    gppProcess.setArguments(newGppArgs);

    // 启动 g++
    gppProcess.start();
    if (!gppProcess.waitForStarted())
    {
        cerr << U8("错误：启动 g++ 失败！原因：") << U8(gppProcess.errorString().toLocal8Bit().data()) << endl;
        for (const QString &tempCpp : tempCppFiles)
        {
            QFile::remove(tempCpp);
        }
        return 1;
    }

    // 实时读取并输出日志（Qt 5.9 兼容）
    QByteArray buffer;
    while (gppProcess.state() == QProcess::Running)
    {
        if (gppProcess.waitForReadyRead(100))
        {
            buffer = gppProcess.readAll();
            cout << U8(buffer.data());
            cout.flush();
        }
    }

    // 读取剩余输出
    buffer = gppProcess.readAll();
    if (!buffer.isEmpty())
    {
        cout << U8(buffer.data());
        cout.flush();
    }

    // 等待进程结束并获取退出码
    gppProcess.waitForFinished();
    int compileExitCode = gppProcess.exitCode();

    // 步骤 6：清理临时文件（关键！避免残留）
    for (const QString &tempCpp : tempCppFiles)
    {
        QFile::remove(tempCpp);
        if (QFile::exists(tempCpp))
        {
            cerr << U8("警告：临时文件清理失败 - ") << U8(tempCpp.toLocal8Bit().data()) << endl;
        }
    }
    // 步骤 7：编译成功则执行 a.out
    if (compileExitCode == 0)
    {
        int exeExitCode = executeAOut();  // 执行默认生成的 a.out
        return exeExitCode;               // 返回执行退出码
    }
    else
    {
        cerr << U8("错误：编译失败") << endl;
    }

    return compileExitCode;
}

QImage createImage(int width, int height, int grayValue)
{
    // 合法性检查：确保分辨率为正数，灰度值在有效范围
    if (width <= 0 || height <= 0)
    {
        return QImage();  // 非法分辨率返回空图片
    }

    // 限制灰度值在 0-255 范围内
    grayValue = qBound(0, grayValue, 255);

    // 创建 ARGB32 格式图片（Qt推荐格式，支持透明且绘制高效）
    QImage image(width, height, QImage::Format_ARGB32);

    // 填充灰色背景（alpha通道设为255，完全不透明）
    QColor grayColor(grayValue, grayValue, grayValue, 255);
    image.fill(grayColor.rgba());

    return image;
}

QImage generateDinosaurImage(int width, int height, const QColor &bgColor, const QColor &mainColor)
{
    // 1. 参数校验
    if (width < 100 || height < 100)
    {
        qDebug() << "错误：图像宽高需≥100像素！当前宽：" << width << " 高：" << height;
        return QImage();
    }

    // 2. 初始化图像（显式设置不透明背景，强制alpha通道为255）
    QImage dinosaurImage(width, height, QImage::Format_ARGB32);
    // 关键修正1：确保背景色完全不透明
    QColor solidBgColor = bgColor;
    solidBgColor.setAlpha(255);  // 强制背景不透明
    dinosaurImage.fill(solidBgColor);

    // 3. 创建绘制器（启用反锯齿，保证绘制质量）
    QPainter painter(&dinosaurImage);
    if (!painter.isActive())
    {
        qDebug() << "错误：QPainter初始化失败！";
        return QImage();
    }
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    // 关键修正2：设置绘制模式为覆盖（避免透明混合）
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // 4. 计算绘制比例（适配任意宽高，恐龙占画布70%的最小边）
    const qreal minSide = qMin(width, height);
    const qreal scale   = minSide * 0.7 / 1000;  // 基准尺寸1000，缩放至画布70%
    const qreal centerX = width / 2.0;           // 画布中心X
    const qreal centerY = height / 2.0;          // 画布中心Y

    // 5. 生成渐变颜色（基于主色自动计算深浅渐变，强制不透明）
    QColor lightColor  = mainColor.lighter(130);
    QColor darkColor   = mainColor.darker(120);
    QColor borderColor = mainColor.darker(150);
    // 强制所有绘制颜色不透明
    lightColor.setAlpha(255);
    darkColor.setAlpha(255);
    borderColor.setAlpha(255);

    // -------------------------- 绘制恐龙身体 --------------------------
    QLinearGradient bodyGradient(centerX - 300 * scale, centerY - 150 * scale, centerX + 300 * scale,
                                 centerY + 150 * scale);
    bodyGradient.setColorAt(0, darkColor);
    bodyGradient.setColorAt(1, lightColor);
    painter.setBrush(QBrush(bodyGradient));
    painter.setPen(QPen(borderColor, 15 * scale));

    QRectF bodyRect(centerX - 300 * scale, centerY - 150 * scale, 600 * scale, 300 * scale);
    painter.drawEllipse(bodyRect);

    // -------------------------- 绘制恐龙头部 --------------------------
    QLinearGradient headGradient(centerX + 250 * scale, centerY - 200 * scale, centerX + 350 * scale,
                                 centerY - 100 * scale);
    headGradient.setColorAt(0, darkColor.darker(110));
    headGradient.setColorAt(1, lightColor.lighter(110));
    painter.setBrush(QBrush(headGradient));
    painter.setPen(QPen(borderColor, 12 * scale));

    QRectF headRect(centerX + 250 * scale, centerY - 200 * scale, 200 * scale, 180 * scale);
    painter.drawRoundedRect(headRect, 30 * scale, 30 * scale);

    // 眼睛（强制黑色不透明）
    painter.setBrush(QColor(0, 0, 0, 255));  // 显式设置不透明黑色
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(centerX + 320 * scale, centerY - 160 * scale, 25 * scale, 25 * scale);

    // 嘴巴（强制红色不透明）
    painter.setPen(QPen(QColor(255, 0, 0, 255), 8 * scale));  // 显式不透明红色
    painter.drawLine(QPointF(centerX + 380 * scale, centerY - 120 * scale),
                     QPointF(centerX + 420 * scale, centerY - 100 * scale));

    // -------------------------- 绘制恐龙四肢 --------------------------
    painter.setBrush(QBrush(bodyGradient));
    painter.setPen(QPen(borderColor, 15 * scale));

    // 左前腿
    QRectF leftFrontLeg(centerX + 100 * scale, centerY + 150 * scale, 80 * scale, 200 * scale);
    painter.drawRoundedRect(leftFrontLeg, 20 * scale, 20 * scale);

    // 右前腿
    QRectF rightFrontLeg(centerX + 250 * scale, centerY + 150 * scale, 80 * scale, 200 * scale);
    painter.drawRoundedRect(rightFrontLeg, 20 * scale, 20 * scale);

    // 左后腿
    QRectF leftBackLeg(centerX - 200 * scale, centerY + 150 * scale, 80 * scale, 200 * scale);
    painter.drawRoundedRect(leftBackLeg, 20 * scale, 20 * scale);

    // 右后腿
    QRectF rightBackLeg(centerX - 80 * scale, centerY + 150 * scale, 80 * scale, 200 * scale);
    painter.drawRoundedRect(rightBackLeg, 20 * scale, 20 * scale);

    // -------------------------- 绘制恐龙尾巴 --------------------------
    // 关键修正3：尾巴不仅绘制轮廓，还要填充颜色
    painter.setPen(QPen(borderColor, 20 * scale, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(bodyGradient));  // 增加填充
    QPainterPath tailPath;
    tailPath.moveTo(centerX - 300 * scale, centerY);
    tailPath.cubicTo(centerX - 600 * scale, centerY - 200 * scale, centerX - 800 * scale, centerY + 100 * scale,
                     centerX - 700 * scale, centerY + 200 * scale);
    // 关键修正4：使用drawPath+填充（或drawPolygon）确保不透明
    painter.drawPath(tailPath);
    // 额外保障：填充尾巴路径区域
    painter.fillPath(tailPath, QBrush(bodyGradient));

    // 关键修正5：结束绘制前重置画笔/画刷，避免状态污染

    return dinosaurImage;
}
