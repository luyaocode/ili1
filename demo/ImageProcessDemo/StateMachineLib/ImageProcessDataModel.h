#ifndef IMAGEPROCESSDATAMODEL_H
#define IMAGEPROCESSDATAMODEL_H

#include <QScxmlCppDataModel>
#include <QTimer>

// 继承 QScxmlCppDataModel，实现 C++ 数据模型
class ImageProcessDataModel : public QScxmlCppDataModel
{
    Q_OBJECT
    // 注册数据模型类，供 SCXML 状态机使用
    Q_SCXML_DATAMODEL

public:
    explicit ImageProcessDataModel(QObject *parent = nullptr);
    ~ImageProcessDataModel() override = default;

    // 数据访问接口（供 SCXML 状态机调用）
    QString setImagePath(const QString &path);
    int resetProgress();
    void startProgressTimer();
    void stopProgressTimer();

private slots:
    // 定时器触发：更新进度
    void onProgressTimerTimeout();

private:
    // 数据模型成员变量（与 SCXML 中的 <data> 对应）
    QString m_imagePath;
    int m_processProgress;
    QString m_processResult;

    // 进度更新定时器
    QTimer m_progressTimer;
};

#endif // IMAGEPROCESSDATAMODEL_H
