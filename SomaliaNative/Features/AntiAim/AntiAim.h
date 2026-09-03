#pragma once
#include <windows.h>
#include <cstdint>

namespace AntiAim
{
    void Initialize();
    void Update();
    void Reset();

    int GetChokedTicks();
    float GetRealAngle();
    float GetFakeAngle();
    bool IsActive();
    bool IsInvertebredActive();
    bool ShouldChokeSyncPacket();
}
