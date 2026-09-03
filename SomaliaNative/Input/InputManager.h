#pragma once
#include <windows.h>

namespace InputManager
{
    void Initialize(HWND hWnd);
    void RestoreWndProc();
    void Shutdown();
    void SetMenuCursorState(bool enabled);
    void ToggleMenu(bool open);
    bool IsMenuOpen();
}
