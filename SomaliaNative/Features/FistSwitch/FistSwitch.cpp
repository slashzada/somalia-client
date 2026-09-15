#include "FistSwitch.h"
#include "../../Config/Config.h"
#include "../../Core/RuntimeState.h"
#include "../../Core/Logger.h"
#include "../../Engine/GTA/GTA.h"
#include <thread>
#include <atomic>

namespace FistSwitch
{
    static std::atomic<bool> s_CleoScriptFound(false);
    static bool              s_WasAiming = false;
    static std::atomic<bool> s_SwitchInProgress(false);

    typedef void(__thiscall* tCPed_SetCurrentWeapon)(void*, int);
    static auto pSetCurrentWeapon = reinterpret_cast<tCPed_SetCurrentWeapon>(0x005E6280);

    static bool IsCharInAir(void* pPed)
    {
        if (!pPed) return false;
        __try
        {
            uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pPed);
            uint8_t flags46D = *reinterpret_cast<uint8_t*>(pedAddr + 0x46D);
            return (flags46D & 0x02) != 0; // bit 1: bIsInTheAir (Opcode 0818)
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    static bool IsCharDriving(void* pPed)
    {
        if (!pPed) return false;
        __try
        {
            uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pPed);
            void* pVeh = *reinterpret_cast<void**>(pedAddr + 0x58C);
            if (pVeh != nullptr) return true;
            uint8_t flags46D = *reinterpret_cast<uint8_t*>(pedAddr + 0x46D);
            return (flags46D & 0x01) != 0; // bit 0: bInVehicle (Opcode 00DF)
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    // Procura o script xxxx.cs na lista de scripts ativos do GTA para sincronizar com o toggle do menu
    static bool SyncCleoActiveScripts(bool enable)
    {
        __try
        {
            void** ppActiveScripts = reinterpret_cast<void**>(0x00A8B42C);
            if (!ppActiveScripts || !*ppActiveScripts) return false;

            uint8_t cleoSignature[] = { 0x01, 0x00, 0x04, 0x00, 0x01, 0x00, 0x04, 0x00, 0x56, 0x02 };
            void* pCurrent = *ppActiveScripts;

            while (pCurrent)
            {
                uint8_t* pBaseIP = *reinterpret_cast<uint8_t**>(reinterpret_cast<uintptr_t>(pCurrent) + 0x10);
                if (pBaseIP && !IsBadReadPtr(pBaseIP, sizeof(cleoSignature)))
                {
                    if (memcmp(pBaseIP, cleoSignature, sizeof(cleoSignature)) == 0)
                    {
                        *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(pCurrent) + 0x38) = enable;
                        s_CleoScriptFound.store(true);
                        return true;
                    }
                }
                pCurrent = *reinterpret_cast<void**>(pCurrent);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return false;
    }

    void Initialize()
    {
        s_CleoScriptFound.store(false);
        s_WasAiming = false;
        s_SwitchInProgress.store(false);
    }

    void Reset()
    {
        s_WasAiming = false;
        s_SwitchInProgress.store(false);
    }

    void Update()
    {
        // 1. Sincroniza estado de ativaÃ§Ã£o com o script CLEO nativo se estiver carregado
        bool foundCleo = SyncCleoActiveScripts(g_MenuState.fistSwitch.enabled);

        if (!g_MenuState.fistSwitch.enabled || g_MenuState.menuOpen)
        {
            s_WasAiming = false;
            return;
        }

        // 2. Se o CLEO oficial jÃ¡ carregou o xxxx.cs no jogo, deixa o CLEO executar nativamente
        if (foundCleo)
        {
            return;
        }

        // 3. Fallback imediato: se o jogo foi injetado sem reiniciar o CLEO, executa a rotina exata de xxxx.cs
        void* pLocalPed = RuntimeState::GetLocalPed();
        if (!pLocalPed || !RuntimeState::IsPlayerAlive())
        {
            s_WasAiming = false;
            return;
        }

        if (IsCharDriving(pLocalPed) || IsCharInAir(pLocalPed))
        {
            s_WasAiming = false;
            return;
        }

        bool isAimingNow = ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);

        // Disparo ao soltar a mira (RMB): mirandoAnteriormente and not mirandoAgora
        if (s_WasAiming && !isAimingNow)
        {
            if (!s_SwitchInProgress.exchange(true))
            {
                std::thread([]()
                {
                    void* pPed = RuntimeState::GetLocalPed();
                    if (pPed)
                    {
                        uint32_t originalWeapon = GTA::GetCurrentWeaponId();
                        if (originalWeapon != 0) // SÃ³ troca se nÃ£o estiver jÃ¡ de punho
                        {
                            // Rotina idÃªntica ao xxxx.cs:
                            // :Noname_119: wait 10, set_weapon 0, wait 10, check 0 or 1
                            for (int retry = 0; retry < 5; ++retry)
                            {
                                Sleep(10);
                                pSetCurrentWeapon(pPed, 0);
                                Sleep(10);
                                uint32_t current = GTA::GetCurrentWeaponId();
                                if (current == 0 || current == 1)
                                    break;
                            }

                            // :Noname_181: set_weapon 0@, wait 10, set_weapon 0, wait 10
                            pSetCurrentWeapon(pPed, originalWeapon);
                            Sleep(10);
                            pSetCurrentWeapon(pPed, 0);
                            Sleep(10);
                        }
                    }
                    s_SwitchInProgress.store(false);
                }).detach();
            }
        }

        s_WasAiming = isAimingNow;
    }
}