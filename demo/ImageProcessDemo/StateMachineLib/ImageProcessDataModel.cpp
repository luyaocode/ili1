#include "ImageProcessDataModel.h"
#include <QScxmlStateMachine>

// 注册数据模型类到 Qt 元对象系统
//Q_SCXML_DATAMODEL_REGISTER(ImageProcessDataModel)

ImageProcessDataModel::ImageProcessDataModel(QObject *parent)
    : QScxmlCppDataModel(parent)
{
    // 配置进度定时器（100ms 触发一次）
    m_progressTimer.setInterval(100);
    connect(&m_progressTimer, &QTimer::timeout,
            this, &ImageProcessDataModel::onProgressTimerTimeout);
}

// 设置图片路径（供 SCXML <assign> 调用）
QString ImageProcessDataModel::setImagePath(const QString &path)
{
    m_imagePath = path;
    return m_imagePath;
}

// 重置进度（供 SCXML <assign> 调用）
int ImageProcessDataModel::resetProgress()
{
    m_processProgress = 0;
    // 通知状态机数据已更新（触发 processProgressChanged 信号）
    notifyChanged("processProgress");
    return m_processProgress;
}

// 启动进度定时器（供 SCXML <onentry> 调用）
void ImageProcessDataModel::startProgressTimer()
{
    m_progressTimer.start();
}

// 停止进度定时器（供 SCXML <onexit> 调用）
void ImageProcessDataModel::stopProgressTimer()
{
    m_progressTimer.stop();
}

// 定时器超时：更新进度
void ImageProcessDataModel::onProgressTimerTimeout()
{
    m_processProgress += 10;
    if (m_processProgress >= 100) {
        m_processProgress = 100;
        m_progressTimer.stop();
        // 进度完成，向状态机提交 finish 事件
        stateMachine()->submitEvent("finish");
    }
    // 通知状态机进度已更新（自动触发生成代码的 processProgressChanged 信号）
    notifyChanged("processProgress");
}
