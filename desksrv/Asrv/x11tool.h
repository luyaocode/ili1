#ifndef X11TOOL_H
#define X11TOOL_H

#include <unordered_map>
#include <string>
#include <vector>
#include <X11/X.h>

class ClientInfo;
// 辅助函数：转换按键名到KeySym
KeySym getKeySymFromName(const std::string &keyName,bool shiftPressed);
// 辅助函数：构建组合键掩码列表
std::vector<KeySym> buildMaskKeys(ClientInfo &info);
#endif // X11TOOL_H
