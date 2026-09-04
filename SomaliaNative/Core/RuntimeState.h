#pragma once
#include <windows.h>
#include <cstdint>

// Enderecos e offsets canonicos para GTA San Andreas 1.0 US (Compact/Hoodlum)
namespace GTASA_10US
{
    constexpr uintptr_t ADDR_LOCAL_PLAYER_PED = 0x00B7CD98; // CWorld::Players[0].m_pPed
    constexpr uintptr_t ADDR_GAME_STATE       = 0x00BA67A4; // CGameState
    constexpr uintptr_t ADDR_CAMERA           = 0x00B6F99C; // TheCamera
    constexpr uintptr_t ADDR_CROSSHAIR_X      = 0x00B6EC14;
    constexpr uintptr_t ADDR_CROSSHAIR_Y      = 0x00B6EC10;
    constexpr uintptr_t ADDR_MOUSE_SENS       = 0x00B6EC1C;

    constexpr uintptr_t PED_OFF_MATRIX        = 0x14;   // RwMatrix*
    constexpr uintptr_t PED_OFF_COORDS        = 0x04;   // CVector
    constexpr uintptr_t PED_OFF_STATE         = 0x530;  // uint32 (54 = PED_STATE_DEAD, 55 = PED_STATE_DIE)
    constexpr uintptr_t PED_OFF_HEALTH        = 0x540;  // float
    constexpr uintptr_t PED_OFF_ARMOR         = 0x548;  // float
}

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

    // Callbacks de transicao
    void OnPlayerDeath();
    void OnPlayerRespawn();
}
