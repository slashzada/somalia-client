#pragma once
#include <windows.h>

namespace StreamProof
{
    void Initialize(HWND hGameWnd);
    void Shutdown();

    void SetEnabled(bool enabled);
    bool IsEnabled();
    void Toggle();

    bool IsSupported();
}
