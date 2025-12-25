#ifndef IMAGEPROCESSSTATE_H
#define IMAGEPROCESSSTATE_H

#include <QObject>
#include "statemachine_global.h"  // 导入导出宏

// 注意：自动生成的状态机头文件仅在库内部包含，主程序不依赖
class QScxmlStateMachine;

class STATEMACHINE_EXPORT ImageProcessState : public QObject
{
    Q_OBJECT
public:
    explicit ImageProcessState(QObject *parent = nullptr);
    ~ImageProcessState() = default;

    // 导出的业务接口（主程序可调用）
    void startStateMachine();
    void submitStartProcess(const QString &imagePath);
    void submitCancelProcess();
    void submitReset();
    QString currentStateName() const;

signals:
    // 导出的信号（主程序可连接）
    void signalProgressUpdated(int progress);
    void signalProcessResult(const QString &result);
    void signalStateChanged(const QString &stateName);

private slots:
    // 库内部槽函数（主程序不可见）
    void onEnteredIdle();
    void onEnteredProcessing();
    void onEnteredCompleted();
    void onProcessProgressChanged(const QVariant &value);

private:
    // 自动生成的状态机实例（库内部持有，主程序无感知）
    QScxmlStateMachine *m_pStateMachine;
};

#endif // IMAGEPROCESSSTATE_H
