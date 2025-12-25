#include <QtTest>
#include <CDBus.h>
// add necessary includes here

class SendDBus : public QObject
{
    Q_OBJECT

public:
    SendDBus();
    ~SendDBus();

private slots:
    // 接收
    void test_autoTestSetHardkeyRecordStatus();
    void test_autoTestHardkey_keyEvent();       // 硬按键
    void test_autoTestHardkey_rotationEvent();  // 旋钮
    void test_autoTestHardkey_tgcEvent();       // 滑条
    // 发送
    void test_autoTestSendHardkeyRecordStatus();    // 反馈录制状态
    void test_autoTestSendRecordKeyEvent();         // 反馈硬按键事件
    // 测试dict struct 参数
    void test_send_single_param();
};

SendDBus::SendDBus()
{

}

SendDBus::~SendDBus()
{

}

void SendDBus::test_autoTestSetHardkeyRecordStatus()
{
    // dbus-send --system --type=signal /org/comen/autoTest org.comen.autoTest.autoTestSetHardkeyRecordStatus boolean:true
    CDBus::sendDbusMessage(DBUS_INTERFACE_AUTO_TEST_SH,"autoTestSetHardkeyRecordStatus",QVariantList{true},DBUS_PATH_AUTO_TEST);
}


void SendDBus::test_autoTestHardkey_keyEvent()
{
    // dbus-send --system --type=signal /org/comen/autoTest org.comen.autoTest.autoTestHardkey string:"KEY_ID_Freeze" array:byte:
    // 注意:array:byte:表示空数组,即便参数为空也要加上,否则信号签名不匹配
    CDBus::sendDbusMessage(DBUS_INTERFACE_AUTO_TEST_SH,"autoTestHardkey",QVariantList{"KEY_ID_Freeze",QByteArray()},DBUS_PATH_AUTO_TEST);
}

void SendDBus::test_autoTestHardkey_rotationEvent()
{
    // dbus-send --system --type=signal /org/comen/autoTest org.comen.autoTest.autoTestHardkey string:"KEY_ID_ROTATION_ZOOM" array:byte:0x10
    // 0x10-正转 0x01-反转
    QByteArray bytes;
    bytes.append(0x10);
    CDBus::sendDbusMessage(DBUS_INTERFACE_AUTO_TEST_SH,"autoTestHardkey",QVariantList{"KEY_ID_ROTATION_ZOOM",bytes},DBUS_PATH_AUTO_TEST);
}

void SendDBus::test_autoTestHardkey_tgcEvent()
{
    // dbus-send --system --type=signal /org/comen/autoTest org.comen.autoTest.autoTestHardkey string:"TGC" array:byte:0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7
    QByteArray bytes;
    bytes.append(static_cast<char>(0))
         .append(static_cast<char>(0x1))
         .append(static_cast<char>(0x2))
         .append(static_cast<char>(0x3))
         .append(static_cast<char>(0x4))
         .append(static_cast<char>(0x5))
         .append(static_cast<char>(0x6))
         .append(static_cast<char>(0x7));
    CDBus::sendDbusMessage(DBUS_INTERFACE_AUTO_TEST_SH,"autoTestHardkey",QVariantList{"TGC",bytes},DBUS_PATH_AUTO_TEST);
}

// 监听 dbus-monitor --system "type='signal',path='/org/comen/autoTest',interface='org.comen.autoTest',member='hardkeyRecordStatus'"
void SendDBus::test_autoTestSendHardkeyRecordStatus()
{
    CDBus::sendDbusMessage(DBUS_INTERFACE_AUTO_TEST_SH, "hardkeyRecordStatus", QVariantList() << true, DBUS_PATH_AUTO_TEST);
}

// 监听 dbus-monitor --system "type='signal',path='/org/comen/autoTest',interface='org.comen.autoTest',member='hardKeyTriggered'"
void SendDBus::test_autoTestSendRecordKeyEvent()
{
    {
        CDBus::sendDbusMessage(DBUS_INTERFACE_AUTO_TEST_SH,"hardKeyTriggered",QVariantList{"KEY_ID_Freeze",QByteArray()},DBUS_PATH_AUTO_TEST);
    }
    {
        QByteArray bytes;
        bytes.append(0x10);
        CDBus::sendDbusMessage(DBUS_INTERFACE_AUTO_TEST_SH,"hardKeyTriggered",QVariantList{"KEY_ID_ROTATION_ZOOM",bytes},DBUS_PATH_AUTO_TEST);
    }
    {
        QByteArray bytes;
        bytes.append(static_cast<char>(0))
             .append(static_cast<char>(0x1))
             .append(static_cast<char>(0x2))
             .append(static_cast<char>(0x3))
             .append(static_cast<char>(0x4))
             .append(static_cast<char>(0x5))
             .append(static_cast<char>(0x6))
             .append(static_cast<char>(0x7));
        CDBus::sendDbusMessage(DBUS_INTERFACE_AUTO_TEST_SH, "hardKeyTriggered",QVariantList{"TGC",bytes}, DBUS_PATH_AUTO_TEST);
    }

}

void SendDBus::test_send_single_param()
{
    // dbus-send --system --type=signal /org/aili1/test org.aili1.test.testParam dict:string:int32:"width",1920,"height",1080
    CDBus::sendDbusMessage("org.aili1.test", "testParam", QVariantList() << double_t(110), "/org/aili1/test");
    // dbus-monitor --system "type='signal',path='/org/aili1/test',interface='org.aili1.test',member='testParam'"
}


QTEST_APPLESS_MAIN(SendDBus)

#include "tst_senddbus.moc"
