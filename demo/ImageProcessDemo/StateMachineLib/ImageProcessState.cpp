#include "ImageProcessState.h"

#include <QScxmlStateMachine>
// 仅在库内部包含自动生成的状态机头文件
#include "ImageProcess.h"

ImageProcessState::ImageProcessState(QObject *parent)
    : QObject(parent)
{
    // 创建自动生成的状态机实例（库内部管理生命周期）
    m_pStateMachine = new ImageProcess(this);

    // 连接自动生成的状态信号和数据模型信号

}

// 启动状态机
void ImageProcessState::startStateMachine()
{
    if (m_pStateMachine->isRunning()) return;
    m_pStateMachine->start();
}

// 提交开始处理事件
void ImageProcessState::submitStartProcess(const QString &imagePath)
{
    QVariantMap eventData;
    eventData["imagePath"] = imagePath;
    m_pStateMachine->submitEvent("startProcess", eventData);
}

// 提交取消处理事件
void ImageProcessState::submitCancelProcess()
{
    m_pStateMachine->submitEvent("cancelProcess");
}

// 提交重置事件
void ImageProcessState::submitReset()
{
    m_pStateMachine->submitEvent("reset");
}

// 获取当前状态名
QString ImageProcessState::currentStateName() const
{
    QStringList activeStates = m_pStateMachine->activeStateNames();
    return activeStates.isEmpty() ? "" : activeStates.first();
}

// 内部槽函数：进入空闲状态
void ImageProcessState::onEnteredIdle()
{
    emit signalStateChanged("空闲");
}

// 内部槽函数：进入处理状态
void ImageProcessState::onEnteredProcessing()
{
    emit signalStateChanged("处理中");
}

// 内部槽函数：进入完成状态
void ImageProcessState::onEnteredCompleted()
{
    QString result = m_pStateMachine->getProcessResult().toString();
    emit signalProcessResult(result);
    emit signalStateChanged("处理完成");
}

// 内部槽函数：进度更新
void ImageProcessState::onProcessProgressChanged(const QVariant &value)
{
    emit signalProgressUpdated(value.toInt());
}
