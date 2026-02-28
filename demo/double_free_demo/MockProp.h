#ifndef DOUBLEFREETEST_H
#define DOUBLEFREETEST_H
#include <QCoreApplication>
#include <QList>
#include <QDebug>
#include <QTimer>

// 模拟DBus属性类（QObject子类，小尺寸对象易触发地址复用）
class MockProp : public QObject {
    Q_OBJECT
public:
    MockProp() {
        qDebug() << QString("✅ 新建MockProp | 地址: %1").arg((quintptr)this, 0, 16);
    }
    ~MockProp() override {
        qDebug() << QString("❌ 销毁MockProp | 地址: %1").arg((quintptr)this, 0, 16);
    }
};

// 全局列表（模拟propertyMonitors）
extern QList<MockProp*> propList;

// 核心：清理标志位（未加保护，故意让清理非原子化）
extern bool isCleaning;

// 定时器1：高频往列表插入新对象（模拟DBus发现设备）
inline void insertNewProp() {
    // 故意不判断清理状态，模拟异步插入干扰
    MockProp* newProp = new MockProp();
    propList.append(newProp);
    qDebug() << QString("插入后列表大小: %1").arg(propList.size());
}

// 定时器2：定时清理列表（模拟20秒一次的清理逻辑）
inline void cleanupPropList() {
    qDebug() << "\n===== 开始清理 =====";
    qDebug() << QString("清理前列表大小: %1").arg(propList.size());

    // 1. 删除列表所有对象（生成野指针）
    qDeleteAll(propList);
    qDebug() << QString("qDeleteAll后列表大小: %1").arg(propList.size());

    // 关键：模拟Qt隐式触发事件循环（qDeleteAll内部会触发）
    // 此时定时器1的插入操作会被执行，往列表插新对象
    qApp->processEvents();

    // 2. 清空列表（此时列表已被插入新对象）
    propList.clear();
    qDebug() << QString("clear后列表大小: %1").arg(propList.size());
    qDebug() << "===== 清理结束 =====\n";
}

#endif  // DOUBLEFREETEST_H
