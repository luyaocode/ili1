#include <iostream>
#include <vector>
#include "commontool/x11struct.h"
#include "commontool/globaltool.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "使用方法: " << argv[0] << " <按键...>" << std::endl;
        return 1;
    }
    std::vector<KeySym> mods;
    KeySym              key = 0;
    for (int i = 1; i < argc; ++i)
    {
        KeySym keysym;
        auto succ = stringToKeysym(argv[i], keysym);
        if (!succ)
        {
            return 1;
        }
        if (i == argc - 1)
        {

            key = keysym;
        }
        else
        {
            mods.push_back(keysym);
        }
    }
    simulateKeyWithMask(mods, key);
    return 0;
}
