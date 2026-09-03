#include "RuntimeState.h"
#include "Logger.h"
#include "../Engine/GTA/GTA.h"
#include "../Features/Aimbot/Aimbot.h"
#include "../Features/Aimbot/AimAssist.h"
#include "../Features/Aimbot/RageBot.h"
#include "../Features/Slide/Slide.h"
#include "../Features/AntiAim/AntiAim.h"
#include "../Features/LocalMods/LocalMods.h"

namespace RuntimeState
{
    static PlayerLifeState s_State = PlayerLifeState::UNKNOWN;
    static void* s_LastLocalPed = nullptr;
    static uint64_t s_LastStateChangeTick = 0;

    void Initialize()
    {
        s_State = PlayerLifeState::UNKNOWN;
        s_LastLocalPed = nullptr;
        s_LastStateChangeTick = GetTickCount64();
        Logger::Log("[SOMALIA][LIFECYCLE] RuntimeState inicializado.");
    }

    PlayerLifeState GetState()
    {
        return s_State;
    }

    const char* GetStateName(PlayerLifeState state)
    {
        switch (state)
        {
        case PlayerLifeState::UNKNOWN: return "UNKNOWN";
        case PlayerLifeState::ALIVE: return "ALIVE";
        case PlayerLifeState::DEAD: return "DEAD";
        case PlayerLifeState::RESPAWNING: return "RESPAWNING";
        case PlayerLifeState::ALIVE_AFTER_RESPAWN: return "ALIVE_AFTER_RESPAWN";
        case PlayerLifeState::DISCONNECTED: return "DISCONNECTED";
        default: return "UNKNOWN";
        }
    }

    bool IsValidPed(void* pPed)
    {
        if (!pPed) return false;
        __try
        {
            if (IsBadReadPtr(pPed, 0x600)) return false;
            void* vtable = *reinterpret_cast<void**>(pPed);
            if (!vtable || IsBadReadPtr(vtable, sizeof(void*))) return false;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    void* GetLocalPed()
    {
        __try
        {
            void** ppLocal = reinterpret_cast<void**>(0x00B7CD98);
            if (!ppLocal || IsBadReadPtr(ppLocal, sizeof(void*))) return nullptr;
            void* pPed = *ppLocal;
            if (!IsValidPed(pPed)) return nullptr;
            return pPed;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }
    }

    bool IsPlayerAlive()
    {
        void* pPed = GetLocalPed();
        if (!pPed) return false;

        __try
        {
            uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pPed);
            float hp = *reinterpret_cast<float*>(pedAddr + 0x540);
            if (hp <= 0.0f) return false;

            uint32_t pedState = *reinterpret_cast<uint32_t*>(pedAddr + 0x530);
            // 54 = PED_STATE_DEAD, 55 = PED_STATE_DIE
            if (pedState == 54 || pedState == 55) return false;

            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool IsRespawning()
    {
        return (s_State == PlayerLifeState::RESPAWNING);
    }

    bool IsDead()
    {
        return (s_State == PlayerLifeState::DEAD);
    }

    void OnPlayerDeath()
    {
        Logger::Log("[SOMALIA][LIFECYCLE] Executando limpeza de morte: resetando todos os modulos de combate.");
        Aimbot::ClearTarget();
        AimAssist::Reset();
        AimAssist::ResetSilentDiagnostic();
        RageBot::Reset();
        Slide::Reset();
        AntiAim::Reset();
        LocalMods::Reset();
        s_LastLocalPed = nullptr;
    }

    void OnPlayerRespawn()
    {
        Logger::Log("[SOMALIA][LIFECYCLE] Respawn concluido: restabelecendo estado operacional.");
        Aimbot::ClearTarget();
        AimAssist::Reset();
        RageBot::Reset();
        Slide::Reset();
        AntiAim::Reset();
    }

    void Update()
    {
        void* pPed = GetLocalPed();
        PlayerLifeState oldState = s_State;
        PlayerLifeState newState = s_State;

        if (!pPed)
        {
            if (s_State == PlayerLifeState::DEAD || s_State == PlayerLifeState::RESPAWNING)
            {
                newState = PlayerLifeState::RESPAWNING;
            }
            else
            {
                newState = PlayerLifeState::DISCONNECTED;
            }
        }
        else
        {
            bool alive = IsPlayerAlive();
            if (alive)
            {
                if (s_State == PlayerLifeState::DEAD || s_State == PlayerLifeState::RESPAWNING)
                {
                    newState = PlayerLifeState::ALIVE_AFTER_RESPAWN;
                }
                else if (s_State == PlayerLifeState::ALIVE_AFTER_RESPAWN)
                {
                    newState = PlayerLifeState::ALIVE;
                }
                else
                {
                    newState = PlayerLifeState::ALIVE;
                }
            }
            else
            {
                if (s_State == PlayerLifeState::ALIVE || s_State == PlayerLifeState::ALIVE_AFTER_RESPAWN)
                {
                    newState = PlayerLifeState::DEAD;
                }
                else if (s_State == PlayerLifeState::DEAD)
                {
                    newState = PlayerLifeState::RESPAWNING;
                }
            }
        }

        if (newState != oldState)
        {
            Logger::Log("[SOMALIA][LIFECYCLE] Transicao: %s -> %s (ped=%p)",
                GetStateName(oldState), GetStateName(newState), pPed);

            s_State = newState;
            s_LastStateChangeTick = GetTickCount64();

            if (newState == PlayerLifeState::DEAD)
            {
                OnPlayerDeath();
            }
            else if (newState == PlayerLifeState::ALIVE_AFTER_RESPAWN ||
                     (oldState == PlayerLifeState::RESPAWNING && newState == PlayerLifeState::ALIVE))
            {
                OnPlayerRespawn();
            }
        }

        s_LastLocalPed = pPed;
    }
}
