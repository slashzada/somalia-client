#include "RuntimeState.h"
#include "Logger.h"
#include "Main.h"
#include "../../Common/SafeMemory.h"
#include "../Engine/GTA/GTA.h"
#include "../Features/Aimbot/Aimbot.h"
#include "../Features/Aimbot/AimAssist.h"
#include "../Features/Aimbot/RageBot.h"
#include "../Features/Slide/Slide.h"
#include "../Features/AntiAim/AntiAim.h"
#include "../Features/LocalMods/LocalMods.h"
#include "../Features/Visuals/ESP.h"

namespace RuntimeState
{
    static SRWLOCK s_StateLock = SRWLOCK_INIT;
    static PlayerLifeState s_State = PlayerLifeState::UNKNOWN;
    static void* s_CurrentLocalPed = nullptr;
    static void* s_LastLocalPed = nullptr;
    static uint64_t s_LastStateChangeTick = 0;
    static uint64_t s_NullPedStartTick = 0;

    void Initialize()
    {
        AcquireSRWLockExclusive(&s_StateLock);
        s_State = PlayerLifeState::UNKNOWN;
        s_CurrentLocalPed = nullptr;
        s_LastLocalPed = nullptr;
        s_LastStateChangeTick = GetTickCount64();
        s_NullPedStartTick = 0;
        ReleaseSRWLockExclusive(&s_StateLock);

        Logger::Log("[SOMALIA][LIFECYCLE] RuntimeState inicializado (GTA SA 1.0 US).");
    }

    PlayerLifeState GetState()
    {
        AcquireSRWLockShared(&s_StateLock);
        PlayerLifeState state = s_State;
        ReleaseSRWLockShared(&s_StateLock);
        return state;
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
        if (!SafeMemory::IsValidReadPtr(pPed, 0x550)) return false;

        __try
        {
            void* vtable = *reinterpret_cast<void**>(pPed);
            if (!vtable || !SafeMemory::IsValidReadPtr(vtable, sizeof(void*) * 10)) return false;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    static void* ReadRawLocalPed()
    {
        __try
        {
            void** ppLocal = reinterpret_cast<void**>(GTASA_10US::ADDR_LOCAL_PLAYER_PED);
            if (!ppLocal || !SafeMemory::IsValidReadPtr(ppLocal, sizeof(void*))) return nullptr;
            void* pPed = *ppLocal;
            if (!IsValidPed(pPed)) return nullptr;
            return pPed;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }
    }

    static bool CheckRawPedAlive(void* pPed)
    {
        if (!pPed || !IsValidPed(pPed)) return false;

        __try
        {
            uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pPed);
            float hp = *reinterpret_cast<float*>(pedAddr + GTASA_10US::PED_OFF_HEALTH);
            if (hp <= 0.0f) return false;

            uint32_t pedState = *reinterpret_cast<uint32_t*>(pedAddr + GTASA_10US::PED_OFF_STATE);
            // 54 = PED_STATE_DEAD, 55 = PED_STATE_DIE
            if (pedState == 54 || pedState == 55) return false;

            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    void* GetLocalPed()
    {
        if (Main::IsShuttingDown())
            return nullptr;

        AcquireSRWLockShared(&s_StateLock);
        if (s_State == PlayerLifeState::DEAD ||
            s_State == PlayerLifeState::RESPAWNING ||
            s_State == PlayerLifeState::UNKNOWN ||
            s_State == PlayerLifeState::DISCONNECTED)
        {
            ReleaseSRWLockShared(&s_StateLock);
            return nullptr;
        }

        void* pPed = s_CurrentLocalPed;
        ReleaseSRWLockShared(&s_StateLock);
        return pPed;
    }

    bool IsPlayerAlive()
    {
        if (Main::IsShuttingDown())
            return false;

        AcquireSRWLockShared(&s_StateLock);
        if (s_State == PlayerLifeState::DEAD ||
            s_State == PlayerLifeState::RESPAWNING ||
            s_State == PlayerLifeState::UNKNOWN ||
            s_State == PlayerLifeState::DISCONNECTED)
        {
            ReleaseSRWLockShared(&s_StateLock);
            return false;
        }

        bool alive = (s_State == PlayerLifeState::ALIVE || s_State == PlayerLifeState::ALIVE_AFTER_RESPAWN) &&
                     (s_CurrentLocalPed != nullptr);
        ReleaseSRWLockShared(&s_StateLock);
        return alive;
    }

    bool IsRespawning()
    {
        AcquireSRWLockShared(&s_StateLock);
        bool respawning = (s_State == PlayerLifeState::RESPAWNING);
        ReleaseSRWLockShared(&s_StateLock);
        return respawning;
    }

    bool IsDead()
    {
        AcquireSRWLockShared(&s_StateLock);
        bool dead = (s_State == PlayerLifeState::DEAD);
        ReleaseSRWLockShared(&s_StateLock);
        return dead;
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
        ESP::Reset();
    }

    void OnPlayerRespawn()
    {
        if (Main::IsShuttingDown())
        {
            Logger::Log("[SOMALIA][LIFECYCLE] OnPlayerRespawn ignorado: shutdown em andamento.");
            return;
        }

        Logger::Log("[SOMALIA][LIFECYCLE] Respawn concluido: restabelecendo estado operacional.");
        Aimbot::ClearTarget();
        AimAssist::Reset();
        RageBot::Reset();
        Slide::Reset();
        AntiAim::Reset();
        LocalMods::Reset();
        ESP::Reset();
    }

    void Update()
    {
        if (Main::IsShuttingDown())
            return;

        void* rawPed = ReadRawLocalPed();
        bool rawAlive = CheckRawPedAlive(rawPed);
        uint64_t now = GetTickCount64();

        AcquireSRWLockExclusive(&s_StateLock);

        PlayerLifeState oldState = s_State;
        PlayerLifeState newState = s_State;
        void* newPedContext = nullptr;

        if (!rawPed)
        {
            if (s_State == PlayerLifeState::UNKNOWN)
            {
                newState = PlayerLifeState::UNKNOWN;
                newPedContext = nullptr;
                s_NullPedStartTick = 0;
            }
            else if (s_State == PlayerLifeState::DEAD || s_State == PlayerLifeState::RESPAWNING)
            {
                newState = PlayerLifeState::RESPAWNING;
                newPedContext = nullptr;
                s_NullPedStartTick = 0;
            }
            else if (s_State == PlayerLifeState::ALIVE || s_State == PlayerLifeState::ALIVE_AFTER_RESPAWN)
            {
                // Debounce de ausencia temporaria do ped (interiores, streaming, cutscenes)
                if (s_NullPedStartTick == 0)
                {
                    s_NullPedStartTick = now;
                }

                if (now - s_NullPedStartTick < 400)
                {
                    // Mantem estado ALIVE enquanto temporario, mas ponteiro nulo para evitar crashes
                    newState = s_State;
                    newPedContext = nullptr;
                }
                else
                {
                    // Ped ausente por tempo prolongado: transicao para RESPAWNING confirmada
                    newState = PlayerLifeState::RESPAWNING;
                    newPedContext = nullptr;
                }
            }
            else
            {
                newState = PlayerLifeState::DISCONNECTED;
                newPedContext = nullptr;
                s_NullPedStartTick = 0;
            }
        }
        else
        {
            s_NullPedStartTick = 0;

            if (rawAlive)
            {
                if (s_State == PlayerLifeState::DEAD || s_State == PlayerLifeState::RESPAWNING)
                {
                    newState = PlayerLifeState::ALIVE_AFTER_RESPAWN;
                    newPedContext = rawPed;
                }
                else if (s_State == PlayerLifeState::ALIVE_AFTER_RESPAWN)
                {
                    newState = PlayerLifeState::ALIVE;
                    newPedContext = rawPed;
                }
                else
                {
                    newState = PlayerLifeState::ALIVE;
                    newPedContext = rawPed;
                }
            }
            else
            {
                // rawAlive e falso: morte confirmada no proprio ped (HP <= 0 ou pedState 54/55)
                if (s_State == PlayerLifeState::ALIVE || s_State == PlayerLifeState::ALIVE_AFTER_RESPAWN)
                {
                    newState = PlayerLifeState::DEAD;
                    newPedContext = nullptr;
                }
                else if (s_State == PlayerLifeState::DEAD)
                {
                    newState = PlayerLifeState::RESPAWNING;
                    newPedContext = nullptr;
                }
                else
                {
                    newPedContext = nullptr;
                }
            }
        }

        bool stateChanged = (newState != oldState);
        if (stateChanged)
        {
            s_State = newState;
            s_CurrentLocalPed = newPedContext;
            s_LastStateChangeTick = now;

            if (newState == PlayerLifeState::DEAD)
            {
                s_LastLocalPed = nullptr;
            }
            else if (newState == PlayerLifeState::ALIVE_AFTER_RESPAWN ||
                     (oldState == PlayerLifeState::RESPAWNING && newState == PlayerLifeState::ALIVE))
            {
                s_LastLocalPed = rawPed;
            }
        }
        else
        {
            if (s_State == PlayerLifeState::ALIVE || s_State == PlayerLifeState::ALIVE_AFTER_RESPAWN)
            {
                s_CurrentLocalPed = newPedContext;
            }
            else
            {
                s_CurrentLocalPed = nullptr;
            }
        }

        ReleaseSRWLockExclusive(&s_StateLock);

        if (stateChanged)
        {
            Logger::Log("[SOMALIA][LIFECYCLE] Transicao: %s -> %s",
                GetStateName(oldState), GetStateName(newState));

            if (newState == PlayerLifeState::DEAD ||
                ((oldState == PlayerLifeState::ALIVE || oldState == PlayerLifeState::ALIVE_AFTER_RESPAWN) && newState == PlayerLifeState::RESPAWNING))
            {
                Logger::Log("[SOMALIA][LIFECYCLE] LocalPed invalidado por morte/respawn.");
                OnPlayerDeath();
            }
            else if (newState == PlayerLifeState::ALIVE_AFTER_RESPAWN ||
                     (oldState == PlayerLifeState::RESPAWNING && newState == PlayerLifeState::ALIVE))
            {
                OnPlayerRespawn();
            }
        }
    }
}
