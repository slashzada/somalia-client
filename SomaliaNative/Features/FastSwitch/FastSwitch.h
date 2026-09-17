#pragma once
#include <windows.h>
#include <string>

namespace FastSwitch
{
    void Initialize();
    void Reset();
    void Update();
    bool IsScriptFound();
    void Cleanup();
}
