#pragma once
#include <windows.h>
#include <cstdint>

namespace RuntimeState
{
    enum class PlayerLifeState
    {
        UNKNOWN = 0,
        ALIVE,
        DEAD,
        RESPAWNING,
        ALIVE_AFTER_RESPAWN,
        DISCONNECTED
    };

    void Initialize();
    void Update();

    PlayerLifeState GetState();
    const char* GetStateName(PlayerLifeState state);

    bool IsPlayerAlive();
    bool IsRespawning();
    bool IsDead();

    void* GetLocalPed();
    bool IsValidPed(void* pPed);

    // Callbacks de transição
    void OnPlayerDeath();
    void OnPlayerRespawn();
}
