#include "DoubleFreeTest.h"

void mockDBusCallback(Parent* parent) {
    qDebug() << "\n【异步回调触发】往propList添加新指针";
    // 往列表中插入新指针（此时qDeleteAll已执行，clear()还没执行）
    propList.append(new MockProp(parent));
}

void reproduceNonAtomic(Parent* parent) {
    // 1. 初始化列表
    propList.append(new MockProp(parent));
    propList.append(new MockProp(parent));
    qDebug() << "初始列表大小:" << propList.size(); // 2

    // 2. 执行qDeleteAll，但在clear()前触发异步回调
    qDeleteAll(propList);
    qDebug() << "qDeleteAll后列表大小:" << propList.size(); // 2（仍有野指针）

    // 模拟：事件循环在qDeleteAll和clear()之间处理异步回调
    // 注：QCoreApplication::processEvents()会处理所有待处理的事件（单线程）
    g_app->processEvents();

    // 3. 执行clear()
    propList.clear();
    qDebug() << "clear()后列表大小:" << propList.size(); // 1（因为回调插入了新指针）

    // 4. 再次qDeleteAll，触发double free
    qDebug() << "\n再次执行qDeleteAll";
    qDeleteAll(propList); // 尝试删除回调插入的新指针，但后续父对象销毁会再删一次
}
