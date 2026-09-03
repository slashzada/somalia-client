#pragma once
#include <windows.h>
#include <string>
#include <d3d9.h>
#include "../../SomaliaNative/Render/ImGui/imgui.h"

namespace LoaderMenu
{
    enum class Screen
    {
        Login,
        Register,
        Dashboard
    };

    void Init(HWND hWnd, IDirect3DDevice9* pDevice = nullptr);
    void SetupFonts();
    void Render();
    Screen GetCurrentScreen();
    void SetCurrentScreen(Screen s);
    void UpdateWindowSize(Screen s);
}
