#include <QCoreApplication>
#include <QStateMachine>
#include <QState>
#include <QFinalState>
#include <QTimer>
#include <QDebug>
#include <QEvent>

// 定义自定义事件类型
class TrafficLightEvent : public QEvent
{
public:
    enum Type {
        TimeoutEvent = QEvent::User + 1  // 超时事件
    };

    TrafficLightEvent(Type type) : QEvent(static_cast<QEvent::Type>(type)) {}
};

// 交通信号灯状态机类
class TrafficLight : public QObject
{
    Q_OBJECT

public:
    TrafficLight(QObject *parent = nullptr) : QObject(parent) {
        // 创建状态机
        machine = new QStateMachine(this);
        // 创建各个状态
        redState = new QState(machine);
        yellowState = new QState(machine);
        greenState = new QState(machine);
        finalState = new QFinalState(machine);

        // 配置红灯状态
        redState->assignProperty(this, "currentState", "Red");
        connect(redState, &QState::entered, this, &TrafficLight::onRedEntered);
        connect(redState, &QState::exited, this, &TrafficLight::onRedExited);

        // 配置黄灯状态
        yellowState->assignProperty(this, "currentState", "Yellow");
        connect(yellowState, &QState::entered, this, &TrafficLight::onYellowEntered);
        connect(yellowState, &QState::exited, this, &TrafficLight::onYellowExited);

        // 配置绿灯状态
        greenState->assignProperty(this, "currentState", "Green");
        connect(greenState, &QState::entered, this, &TrafficLight::onGreenEntered);
        connect(greenState, &QState::exited, this, &TrafficLight::onGreenExited);

        // 设置状态转换规则
        // 红灯 -> 绿灯 (3秒后)
        redState->addTransition(this, &TrafficLight::timeout, greenState);
        // 绿灯 -> 黄灯 (3秒后)
        greenState->addTransition(this, &TrafficLight::timeout, yellowState);
        // 黄灯 -> 红灯 (1秒后)
        yellowState->addTransition(this, &TrafficLight::timeout, redState);

        // 运行5个周期后进入最终状态
        connect(this, &TrafficLight::cycleCompleted, [this]() {
            static int cycleCount = 0;
            if (++cycleCount >= 3) {
                qDebug() << "已运行3个周期，状态机即将停止";
                m_shouldStop = true; // 仅标记需要停止
            }
        });

        // 状态机完成时退出程序
        connect(machine, &QStateMachine::finished, qApp, &QCoreApplication::quit);

        // 设置初始状态
        machine->setInitialState(redState);
    }

    // 启动状态机
    void start() {
        machine->start();
    }

    // 获取当前状态
    QString getCurrentState() const {
        return currentState;
    }

signals:
    void timeout();          // 超时信号
    void cycleCompleted();   // 一个周期完成信号

private slots:
    // 进入红灯状态
    void onRedEntered() {
        qDebug() << "进入红灯状态 - 禁止通行";
        startTimer(3000);  // 红灯持续3秒
    }

    // 退出红灯状态
    void onRedExited() {
        qDebug() << "退出红灯状态";
    }

    // 进入黄灯状态
    void onYellowEntered() {
        qDebug() << "进入黄灯状态 - 准备停止";
        startTimer(1000);  // 黄灯持续1秒
    }

    // 退出黄灯状态
    void onYellowExited() {
        qDebug() << "退出黄灯状态";
        emit cycleCompleted();  // 一个完整周期完成
    }

    // 进入绿灯状态
    void onGreenEntered() {
        qDebug() << "进入绿灯状态 - 可以通行";
        startTimer(3000);  // 绿灯持续3秒
    }

    // 退出绿灯状态
    void onGreenExited() {
        qDebug() << "退出绿灯状态";
    }

private:
    QStateMachine *machine = nullptr;
    QState *redState = nullptr;
    QState *yellowState = nullptr;
    QState *greenState = nullptr;
    QFinalState *finalState = nullptr;
    QString currentState;
    QTimer *timer = nullptr;
    bool m_shouldStop = false;

    // 启动定时器
    void startTimer(int msec) {
        if (timer) {
            timer->stop();
            timer->deleteLater();
        }
        timer = new QTimer(this);
        timer->setSingleShot(true);
        // 关键修改：在超时后先检查是否需要停止
        connect(timer, &QTimer::timeout, this, [this]() {
            if (m_shouldStop) {
                // 创建初始状态（临时过渡状态）
                QState* initialState = new QState(machine);
                // 核心：初始状态无条件跳转至终态（无触发事件，立即执行）
                initialState->addTransition(finalState);
                // 设置初始状态为我们创建的initialState
                machine->setInitialState(initialState);
            } else {
                emit timeout(); // 正常触发状态转换
            }
        });
        timer->start(msec);
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qDebug() << "Qt状态机示例 - 交通信号灯模拟";
    qDebug() << "---------------------------------";

    TrafficLight trafficLight;
    trafficLight.start();

    return a.exec();
}

#include "main.moc"
