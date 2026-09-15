#pragma once
#include <windows.h>
#include <cstdint>

namespace KFCSlide
{
    void Initialize();
    void Reset();
    void Update();
    bool IsActive();
    float GetCurrentSpeed();
}
