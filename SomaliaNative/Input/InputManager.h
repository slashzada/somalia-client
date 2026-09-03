#pragma once
#include <windows.h>

namespace InputManager
{
    void Initialize(HWND hWnd);
    void Shutdown();
    void SetMenuCursorState(bool enabled);
    void ToggleMenu(bool open);
    bool IsMenuOpen();
}
