#pragma once
#include <windows.h>
#include <string>

namespace LuaSlide
{
    void Initialize();
    void Reset();
    void Update();
    void SyncToIni();
    void ToggleFromMenu();
    bool IsIniFound();
    void Cleanup();
}
