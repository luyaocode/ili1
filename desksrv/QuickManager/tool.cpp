#include "tool.h"
#include "commontool/keyblocker.h"

void createConfigFile(const QString &filePath)
{
    QFile file(filePath);
    if (file.exists())
    {
        return;
    }
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Failed to create new config file:" << filePath;
        return;
    }
    QJsonArray array;

    // Tool
    QJsonObject monitor;
    monitor["name"]     = "电脑使用记录";
    monitor["type"]     = int(QuickType::Tool);
    monitor["path"]     = "/usr/bin/monitor -q -g";
    monitor["shortcut"] = CONTROL_MASK "+" SHIFT_MASK "+"
                                       "F1";
    monitor["enable"]   = true;

    QJsonObject screenRecorder;
    screenRecorder["type"]     = int(QuickType::Tool);
    screenRecorder["name"]     = "录屏工具";
    screenRecorder["path"]     = "/usr/bin/ScreenRecorder";
    screenRecorder["shortcut"] = CONTROL_MASK "+" SHIFT_MASK "+"
                                              "F2";
    screenRecorder["enable"]   = true;

    QJsonObject stegoTool;
    stegoTool["type"]     = int(QuickType::Tool);
    stegoTool["name"]     = "图片隐写工具";
    stegoTool["path"]     = "/usr/bin/StegoTool";
    stegoTool["shortcut"] = CONTROL_MASK "+" SHIFT_MASK "+"
                                         "F3";
    stegoTool["enable"]   = true;

    QJsonObject xmlEditor;
    xmlEditor["type"]     = int(QuickType::Tool);
    xmlEditor["name"]     = "XML编辑器";
    xmlEditor["path"]     = "/usr/bin/ConfigTool";
    xmlEditor["shortcut"] = CONTROL_MASK "+" SHIFT_MASK "+"
                                         "F4";
    xmlEditor["enable"]   = true;

    QJsonObject pasteFlow;
    pasteFlow["type"]     = int(QuickType::Tool);
    pasteFlow["name"]     = "历史剪切板";
    pasteFlow["path"]     = "/usr/bin/PasteFlow";
    pasteFlow["shortcut"] = CONTROL_MASK "+" SHIFT_MASK "+"
                                         "X";
    pasteFlow["enable"]   = false;

    array.append(monitor);
    array.append(screenRecorder);
    array.append(stegoTool);
    array.append(xmlEditor);
    array.append(pasteFlow);

    // Common
    QJsonObject obj;
    obj["type"]     = int(QuickType::Common);
    obj["shortcut"] = "";

    obj["name"]   = "HOME";
    obj["path"]   = "~";
    obj["enable"] = true;
    array.append(obj);
    obj["name"]   = "项目代码";
    obj["path"]   = "~/codes";
    obj["enable"] = true;
    array.append(obj);
    obj["name"]   = "构建目标";
    obj["path"]   = "~/target_dir";
    obj["enable"] = true;
    array.append(obj);
    obj["name"]   = "构建中间文件";
    obj["path"]   = "~/build_dir";
    obj["enable"] = true;
    array.append(obj);
    obj["name"]   = "三方库";
    obj["path"]   = "~/common_lib";
    obj["enable"] = true;
    array.append(obj);
    obj["name"]   = "共享目录";
    obj["path"]   = "~/share";
    obj["enable"] = true;
    array.append(obj);
    obj["name"]   = "ili1代码";
    obj["path"]   = "~/ili1";
    obj["enable"] = true;
    array.append(obj);
    obj["name"]   = "测试用例";
    obj["path"]   = "~/testcase_dir";
    obj["enable"] = true;
    array.append(obj);
    obj["name"]   = "安装包";
    obj["path"]   = "~/install_package";
    obj["enable"] = true;
    array.append(obj);

    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    file.close();
    qDebug() << "Config file created: " << filePath;
}

bool getKeyAndModifer(const QuickItem &item, KeyWithModifier &keymod)
{
    if(item.shortcut.isEmpty())
    {
        return false;
    }
    unsigned int mod  = 0;
    QStringList  keys = item.shortcut.split('+');
    if (keys.contains(SHIFT_MASK))
    {
        mod |= ShiftMask;
        keys.removeAll(SHIFT_MASK);
    }
    if (keys.contains(CONTROL_MASK))
    {
        mod |= ControlMask;
        keys.removeAll(CONTROL_MASK);
    }
    if (keys.contains(LOCK_MASK))
    {
        mod |= LockMask;
        keys.removeAll(LOCK_MASK);
    }
    if (keys.contains(MOD1_MASK))
    {
        mod |= Mod1Mask;
        keys.removeAll(MOD1_MASK);
    }
    if (keys.contains(MOD2_MASK))
    {
        mod |= Mod2Mask;
        keys.removeAll(MOD2_MASK);
    }
    if (keys.contains(MOD3_MASK))
    {
        mod |= Mod3Mask;
        keys.removeAll(MOD3_MASK);
    }
    if (keys.contains(MOD4_MASK))
    {
        mod |= Mod4Mask;
        keys.removeAll(MOD4_MASK);
    }
    if (keys.contains(MOD5_MASK))
    {
        mod |= Mod5Mask;
        keys.removeAll(MOD5_MASK);
    }
    if (keys.size() != 1 || keys[0].isEmpty())
    {
        return false;
    }
    keymod.modifier = mod;
    keymod.keysym   = XStringToKeysym(keys[0].toStdString().c_str());
    return true;
}
