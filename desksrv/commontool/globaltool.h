#ifndef GLOBALTOOL_H
#define GLOBALTOOL_H
// 该头文件不包含Qt相关头文件,否则会有编译问题
#include <string>
#include <vector>
#include <X11/X.h>
#include <X11/Xlib.h>

class QMimeData;
// 检查是否已有实例在运行
bool isAlreadyRunning(const std::string &name);
bool isWayland();
// X11 环境：判断焦点窗口是否为终端
bool isTerminalWindowX11();
void activateX11PrimarySelection(const QMimeData *mimeData);
// 无效,改用下面的simulateKeyWithMask
int send_x11_key_combination(unsigned int mod_mask, KeySym keysym, int delay_ms = 50);
/**
* 模拟组合键操作（支持掩码+目标键）
* @param maskKeys 掩码键列表（如Ctrl、Shift等，每个元素为X11的Keysym值）
* @param targetKey 目标按键的X11 Keysym值（如XK_v、XK_c等）
* @param delayMs 每个操作步骤的延迟（毫秒），建议50-100ms
* @return 成功返回true，失败返回false
*/
bool simulateKeyWithMask(const std::vector<KeySym> &maskKeys, KeySym targetKey, int delayMs = 50);

// 模拟按下
bool simulatePressKeyWithMask(const std::vector<KeySym> &maskKeys, KeySym targetKey, int delayMs = 50);
// 模拟释放
bool simulateReleaseKeyWithMask(const std::vector<KeySym> &maskKeys, KeySym targetKey, int delayMs = 50);

// 修饰码（modifier）转换为字符串
std::string modifierToString(unsigned int modifier);

// KeySym（键码）转换为字符串
std::string keysymToString(KeySym keysym);
// 字符串转换为KeySym（键码）
bool stringToKeysym(const std::string& str,KeySym& key);

// 组合修饰码+键码为完整字符串（如 "Ctrl+Shift+A"）
std::string keyCombinationToString(unsigned int modifier, KeySym keysym);

// 解析XEvent并返回单行格式化字符串
std::string getKeyEventString(XEvent &event);

// 辅助函数：将KeySym转为小写（兼容大小写）
KeySym toLowerKeySym(KeySym sym);




bool cstring2ul(const char *str,unsigned long& value);

#endif  // GLOBALTOOL_H
