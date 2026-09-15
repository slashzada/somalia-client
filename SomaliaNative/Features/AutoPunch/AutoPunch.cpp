#include "AutoPunch.h"
#include "../../Config/Config.h"
#include "../../Core/RuntimeState.h"
#include "../../Core/Logger.h"
#include "../../Engine/GTA/GTA.h"
#include "../../Engine/SAMP/SAMP.h"
#include <thread>
#include <cstdlib>
#include <cstdio>

namespace AutoPunch
{
    static bool       s_WasAimingRMB = false;
    static ULONGLONG  s_RmbReleaseTick = 0;
    static bool       s_PunchScheduled = false;
    static ULONGLONG  s_LastPunchTick = 0;
    static uint32_t   s_LastSeenWeapon = 0;

    static const uint32_t WEAPON_SNIPER = 34;

    static void DebugLog(const char* fmt, ...)
    {
        char buf[512];
        va_list args;
        va_start(args, fmt);
        vsprintf_s(buf, sizeof(buf), fmt, args);
        va_end(args);
        Logger::Log("[AUTOPUNCH] %s", buf);
    }

    static void ExecutePunch()
    {
        DebugLog("ExecutePunch: INICIADO!");
        std::thread([]()
        {
            __try
            {
                *reinterpret_cast<int16_t*>(0x00B73458 + 0x22) = 255;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            DebugLog("ExecutePunch: LMB + fire GAMEKEY pressionados");

            Sleep(40 + (rand() % 15));

            __try
            {
                *reinterpret_cast<int16_t*>(0x00B73458 + 0x22) = 0;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            DebugLog("ExecutePunch: LMB + fire GAMEKEY soltos. SOCO CONCLUIDO!");

        }).detach();
    }

    void Initialize()
    {
        s_WasAimingRMB = false;
        s_RmbReleaseTick = 0;
        s_PunchScheduled = false;
        s_LastPunchTick = 0;
        s_LastSeenWeapon = 0;
        Logger::Log("[AUTOPUNCH] Modulo inicializado com sucesso (DelayPadrao=60ms, CooldownPadrao=250ms, Sniper ID=%u).",
            (unsigned)WEAPON_SNIPER);
    }

    void Reset()
    {
        s_WasAimingRMB = false;
        s_RmbReleaseTick = 0;
        s_PunchScheduled = false;
        s_LastPunchTick = 0;
        s_LastSeenWeapon = 0;
    }

    void Update()
    {
        if (!g_MenuState.autoPunch.enabled || !RuntimeState::IsPlayerAlive())
        {
            Reset();
            return;
        }

        if (g_MenuState.menuOpen)
        {
            return;
        }

        bool chatOrDialog = false;
        __try
        {
            chatOrDialog = SAMP::HasActiveCursor();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        if (chatOrDialog)
        {
            return;
        }

        void* pLocalPed = RuntimeState::GetLocalPed();
        if (!pLocalPed)
        {
            Reset();
            return;
        }

        __try
        {
            uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pLocalPed);
            void* pVeh = *reinterpret_cast<void**>(pedAddr + 0x58C);
            if (pVeh != nullptr)
            {
                Reset();
                return;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        ULONGLONG currentTick = GetTickCount64();
        bool rmbDownNow = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        uint32_t currentWeapon = GTA::GetCurrentWeaponId();

        if (currentWeapon > 0)
        {
            s_LastSeenWeapon = currentWeapon;
        }

        ULONGLONG delayMs = static_cast<ULONGLONG>(g_MenuState.autoPunch.delayMs > 0 ? g_MenuState.autoPunch.delayMs : 60);
        ULONGLONG cooldownMs = static_cast<ULONGLONG>(g_MenuState.autoPunch.cooldownMs > 0 ? g_MenuState.autoPunch.cooldownMs : 250);

        if (rmbDownNow)
        {
            if (!s_WasAimingRMB)
            {
                if (s_PunchScheduled)
                {
                    DebugLog("RMB PRESSIONADO antes do delay: cancelando agendamento anterior para liberar nova mira.");
                    s_PunchScheduled = false;
                    s_RmbReleaseTick = 0;
                }
                DebugLog("RMB PRESSIONADO. currentWeapon=%u, s_LastSeenWeapon=%u",
                    (unsigned)currentWeapon, (unsigned)s_LastSeenWeapon);
            }
            s_WasAimingRMB = true;
        }
        else
        {
            if (s_WasAimingRMB)
            {
                bool inCooldown = (s_LastPunchTick > 0) && (currentTick - s_LastPunchTick < cooldownMs);
                bool hadSniperRecently = (s_LastSeenWeapon == WEAPON_SNIPER) || (currentWeapon == WEAPON_SNIPER);

                DebugLog("RMB SOLTO! currentWeapon=%u, s_LastSeenWeapon=%u, hadSniper=%d, inCooldown=%d, jaAgendado=%d",
                    (unsigned)currentWeapon, (unsigned)s_LastSeenWeapon,
                    hadSniperRecently ? 1 : 0, inCooldown ? 1 : 0, s_PunchScheduled ? 1 : 0);

                if (!inCooldown && hadSniperRecently && !s_PunchScheduled)
                {
                    DebugLog("✅ AGENDANDO SOCO! Vai disparar em %llums (Cooldown=%llums)",
                        delayMs, cooldownMs);
                    s_RmbReleaseTick = currentTick;
                    s_PunchScheduled = true;
                }
                s_WasAimingRMB = false;
            }
        }

        if (s_PunchScheduled && s_RmbReleaseTick > 0)
        {
            ULONGLONG elapsed = currentTick - s_RmbReleaseTick;
            if (elapsed >= delayMs)
            {
                if (currentWeapon <= 1)
                {
                    DebugLog("⏰ TEMPO ESGOTADO (%llums / %llums)! EXECUTANDO SOCO AGORA!", elapsed, delayMs);
                    s_PunchScheduled = false;
                    s_RmbReleaseTick = 0;
                    s_LastPunchTick = currentTick;
                    ExecutePunch();
                }
                else if (elapsed >= delayMs + 80)
                {
                    DebugLog("⏰ TIMEOUT (%llums): Arma nao trocou para soco a tempo (arma=%u). Cancelando.", elapsed, currentWeapon);
                    s_PunchScheduled = false;
                    s_RmbReleaseTick = 0;
                }
            }
        }
    }
}
