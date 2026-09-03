#pragma once
#include <d3d9.h>
#include "../Render/ImGui/imgui.h"

namespace Menu
{
    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();
    void InvalidateDeviceObjects();
    void CreateDeviceObjects(IDirect3DDevice9* pDevice);
    void Render();
}
