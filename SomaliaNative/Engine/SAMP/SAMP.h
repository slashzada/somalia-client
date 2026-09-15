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
        uint32_t color;
        int team;
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

    uintptr_t GetIsListedOffset();
    int GetLargestPlayerId();
    bool GetRemotePlayer(int index, RemotePlayerData& outData, uintptr_t pPlayerPool = 0);
    bool GetLocalPlayerPosition(float outPos[3]);
    bool IsTeammate(int index);
    uint32_t GetLocalPlayerColor();
    uint32_t GetPlayerColor(int playerId);

    enum class TeardownStatus
    {
        NotHooked = 0,
        Restored,
        FailedSafe,
        FailedUnsafe
    };

    bool EnsureRakHook();
    bool IsRakHooked();
    bool EnsureSendTakeDamageHook();
    bool IsSendTakeDamageHooked();
    void RestoreSendTakeDamageHook();
    void SendGiveDamage(int targetId, float damage, int weaponId, int bodyPart);
    bool SendBulletData(uint16_t targetId, const float origin[3], const float target[3], const float center[3], uint8_t weaponId, uint8_t hitType = 1);
    bool SendRawPacket(const unsigned char* data, int length, int priority = 1, int reliability = 7, char orderingChannel = 0);
    void* GetRakClient();
    TeardownStatus GetTeardownStatus();
    TeardownStatus Shutdown();
}
