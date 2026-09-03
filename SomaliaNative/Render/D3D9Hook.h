#pragma once
#include <windows.h>
#include <d3d9.h>

namespace D3D9Hook
{
    bool Initialize();
    void RestoreHooks();
    void DestroyUI();
    void Shutdown();
}
