#pragma once
#include <windows.h>
#include <stdint.h>

namespace SAMP
{
    enum class Version
    {
        Unknown = 0,
        R1,
        R2,
        R3,
        R4,
        R5,
        DL
    };

    struct RemotePlayerData
    {
        int playerId;
        bool isValid;
        bool isStreamed;
        uint32_t gtaPedHandle;
        void* pGtaPed;
        float position[3];
        float health;
        float armor;
        char name[32];
    };

    bool IsLoaded();
    uintptr_t GetBaseAddress();
    Version GetVersion();
    const char* GetVersionString();
    void ToggleCursor(bool enable);
    bool HasActiveCursor();

    uintptr_t GetSAMPInfo();
    uintptr_t GetPools();
    uintptr_t GetPlayerPool();
    uint16_t GetLocalPlayerId();
    uintptr_t GetLocalPlayer();
    uintptr_t GetLocalPlayerOnFootData();

    bool GetRemotePlayer(int index, RemotePlayerData& outData);
    bool GetLocalPlayerPosition(float outPos[3]);

    enum class TeardownStatus
    {
        NotHooked = 0,
        Restored,
        FailedSafe,
        FailedUnsafe
    };

    bool EnsureRakHook();
    bool IsRakHooked();
    TeardownStatus GetTeardownStatus();
    TeardownStatus Shutdown();
}
