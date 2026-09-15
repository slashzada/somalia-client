#pragma once
#include <windows.h>
#include <stdint.h>

namespace TriggerBot
{
    void Initialize();
    void Update();
    void Reset();
    bool IsTargetUnderCrosshair(int& outTargetPlayerId);
}
