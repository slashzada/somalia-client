#pragma once
#include <windows.h>

namespace Main
{
    void RequestUnload();
    bool IsShuttingDown();
    bool IsUnloaded();
    HMODULE GetModuleInstance();
    void UnloadAndExit();
}
