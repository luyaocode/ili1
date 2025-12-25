#include "ClientInfo.h"
#include "x11tool.h"
#include "X11/keysym.h"

const std::unordered_map<std::string, KeySym> g_keySymMap = {
    // 基础按键
    {"a", XK_a},
    {"b", XK_b},
    {"c", XK_c},
    {"d", XK_d},
    {"e", XK_e},
    {"f", XK_f},
    {"g", XK_g},
    {"h", XK_h},
    {"i", XK_i},
    {"j", XK_j},
    {"k", XK_k},
    {"l", XK_l},
    {"m", XK_m},
    {"n", XK_n},
    {"o", XK_o},
    {"p", XK_p},
    {"q", XK_q},
    {"r", XK_r},
    {"s", XK_s},
    {"t", XK_t},
    {"u", XK_u},
    {"v", XK_v},
    {"w", XK_w},
    {"x", XK_x},
    {"y", XK_y},
    {"z", XK_z},
    {"0", XK_0},
    {"1", XK_1},
    {"2", XK_2},
    {"3", XK_3},
    {"4", XK_4},
    {"5", XK_5},
    {"6", XK_6},
    {"7", XK_7},
    {"8", XK_8},
    {"9", XK_9},

    // 功能按键
    {"space", XK_space},
    {"enter", XK_Return},
    {"tab", XK_Tab},
    {"backspace", XK_BackSpace},
    {"delete", XK_Delete},
    {"esc", XK_Escape},
    {"arrowup", XK_Up},
    {"arrowdown", XK_Down},
    {"arrowleft", XK_Left},
    {"arrowright", XK_Right},
    {"home", XK_Home},
    {"end", XK_End},
    {"pageup", XK_Page_Up},
    {"pagedown", XK_Page_Down},
    {"capslock", XK_Caps_Lock},
    {"shift", XK_Shift_L},
    {"ctrl", XK_Control_L},
    {"alt", XK_Alt_L},
    {"meta", XK_Super_L},  // Windows/Command键
    {"win", XK_Super_L},   // Windows/Command键

    // F1-F12
    {"f1", XK_F1},
    {"f2", XK_F2},
    {"f3", XK_F3},
    {"f4", XK_F4},
    {"f5", XK_F5},
    {"f6", XK_F6},
    {"f7", XK_F7},
    {"f8", XK_F8},
    {"f9", XK_F9},
    {"f10", XK_F10},
    {"f11", XK_F11},
    {"f12", XK_F12},

    // 符号键
    {"minus", XK_minus},
    {"equal", XK_equal},
    {"bracketleft", XK_bracketleft},
    {"bracketright", XK_bracketright},
    {"backslash", XK_backslash},
    {"semicolon", XK_semicolon},
    {"apostrophe", XK_apostrophe},
    {"comma", XK_comma},
    {"period", XK_period},
    {"slash", XK_slash},
    {"grave", XK_grave},

    // 新增：符号键（Shift+普通键）对应的KeySym
    {"exclam", XK_exclam},            // ! (Shift+1)
    {"at", XK_at},                    // @ (Shift+2)
    {"numbersign", XK_numbersign},    // # (Shift+3)
    {"dollar", XK_dollar},            // $ (Shift+4)
    {"percent", XK_percent},          // % (Shift+5)
    {"asciicircum", XK_asciicircum},  // ^ (Shift+6)
    {"ampersand", XK_ampersand},      // & (Shift+7)
    {"asterisk", XK_asterisk},        // * (Shift+8)
    {"parenleft", XK_parenleft},      // ( (Shift+9)
    {"parenright", XK_parenright},    // ) (Shift+0)
    {"underscore", XK_underscore},    // _ (Shift+-)
    {"plus", XK_plus},                // + (Shift+=)
    {"braceleft", XK_braceleft},      // { (Shift+[)
    {"braceright", XK_braceright},    // } (Shift+])
    {"bar", XK_bar},                  // | (Shift+\)
    {"colon", XK_colon},              // : (Shift+;)
    {"quotedbl", XK_quotedbl},        // " (Shift+')
    {"less", XK_less},                // < (Shift+,)
    {"greater", XK_greater},          // > (Shift+.)
    {"question", XK_question},        // ? (Shift+/)
    {"asciitilde", XK_asciitilde}     // ~ (Shift+`)
};

KeySym getKeySymFromName(const std::string &keyName, bool shiftPressed)
{
    // Step1: 处理Shift+普通键的符号映射
    std::unordered_map<std::string, std::string> shiftKeyMap = {
        {"1", "exclam"},                 // !
        {"2", "at"},                     // @
        {"3", "numbersign"},             // #
        {"4", "dollar"},                 // $
        {"5", "percent"},                // %
        {"6", "asciicircum"},            // ^
        {"7", "ampersand"},              // &
        {"8", "asterisk"},               // *
        {"9", "parenleft"},              // (
        {"0", "parenright"},             // )
        {"minus", "underscore"},         // _
        {"equal", "plus"},               // +
        {"bracketleft", "braceleft"},    // {
        {"bracketright", "braceright"},  // }
        {"backslash", "bar"},            // |
        {"semicolon", "colon"},          // :
        {"apostrophe", "quotedbl"},      // "
        {"comma", "less"},               // <
        {"period", "greater"},           // >
        {"slash", "question"},           // ?
        {"grave", "asciitilde"}          // ~
    };

    // Step2: 如果Shift按下，优先获取符号键的KeySym
    if (shiftPressed && shiftKeyMap.find(keyName) != shiftKeyMap.end())
    {
        auto it = g_keySymMap.find(shiftKeyMap[keyName]);
        if (it != g_keySymMap.end())
        {
            return it->second;
        }
    }

    // Step3: 否则返回基础按键的KeySym
    auto baseIt = g_keySymMap.find(keyName);
    if (baseIt != g_keySymMap.end())
    {
        return baseIt->second;
    }

    // Step4: 未找到返回无效KeySym
    return NoSymbol;
}

std::vector<KeySym> buildMaskKeys(ClientInfo &info)
{
    std::vector<KeySym> maskKeys;

    if (info.isCtrlPressed)
    {
        maskKeys.push_back(XK_Control_L);
    }
    if (info.isShiftPressed)
    {
        maskKeys.push_back(XK_Shift_L);
    }
    if (info.isAltPressed)
    {
        maskKeys.push_back(XK_Alt_L);
    }
    if (info.isMetaPressed)
    {
        maskKeys.push_back(XK_Super_L);
    }

    return maskKeys;
}
