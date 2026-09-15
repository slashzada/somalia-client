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
    bool IsDesyncActive();
    bool IsInvertebredActive();
    bool ShouldChokeSyncPacket();
    void GetNetworkQuaternion(float outQuat[4]);
    void MutateOnFootPacket(unsigned char* data, int length);
    void ProcessDamageRPC(unsigned char* data, int length, int bitCount);
    void ProcessDamageBitStream(unsigned char* data, int bitCount);
}
