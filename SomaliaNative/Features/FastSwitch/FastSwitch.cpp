#include "FastSwitch.h"
#include "../EmbeddedAssets.h"
#include "../../Config/Config.h"
#include "../../Core/Logger.h"
#include "../../Core/RuntimeState.h"
#include "../../Engine/GTA/GTA.h"
#include "../PlayerSlap/PlayerSlap.h"
#include <cstdio>
#include <cstring>

namespace FastSwitch
{
    static bool s_bScriptFound = false;
    static bool s_bLastEnabledState = false;
    static bool s_bInitialized = false;

    // Estado da mÃ¡quina de execuÃ§Ã£o sÃ­ncrona
    static bool s_WasAiming = false;
    static int s_SwitchState = 0; // 0: Idle, 1: Aguardando 10ms pÃ³s soco, 2: Aguardando 10ms pÃ³s restore
    static ULONGLONG s_StateTimer = 0;
    static uint32_t s_OriginalWeapon = 0;

    typedef void(__thiscall* tCPed_SetCurrentWeapon)(void*, int);
    static auto pSetCurrentWeapon = reinterpret_cast<tCPed_SetCurrentWeapon>(0x005E6280);

    // Assinaturas de opcodes para identificaÃ§Ã£o precisa
    static const uint8_t s_Signature10[] = {
        0x01, 0x00, 0x04, 0x00, 0x01, 0x00, 0x04, 0x00, 0x56, 0x02
    };

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

    static void SafeSetCleoGlobalVar(uint32_t val)
    {
        __try
        {
            uint32_t* pFastSwitchGlobal = reinterpret_cast<uint32_t*>(0x00A49960 + 0x7530);
            *pFastSwitchGlobal = val;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void AutoDeployIfMissing()
    {
        DWORD attrib = GetFileAttributesA("cleo");
        if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY))
        {
            // Remove qualquer resíduo antigo de xxxx.cs
            const char* legacyPath = "cleo\\xxxx.cs";
            if (GetFileAttributesA(legacyPath) != INVALID_FILE_ATTRIBUTES)
            {
                SetFileAttributesA(legacyPath, FILE_ATTRIBUTE_NORMAL);
                DeleteFileA(legacyPath);
            }

            const char* scriptPath = "cleo\\arquive.cs";

            // Se o arquivo antigo ja existir, reseta atributos para permitir sobrescrita
            if (GetFileAttributesA(scriptPath) != INVALID_FILE_ATTRIBUTES)
            {
                SetFileAttributesA(scriptPath, FILE_ATTRIBUTE_NORMAL);
            }

            FILE* f = nullptr;
            if (fopen_s(&f, scriptPath, "wb") == 0 && f)
            {
                fwrite(s_ArquiveCsPayload, 1, s_ArquiveCsPayloadSize, f);
                fclose(f);
                Logger::Log("[FAST_SWITCH] arquive.cs atualizado/sobrescrito com sucesso (%zu bytes).", s_ArquiveCsPayloadSize);
            }

            // Aplica atributos de ocultacao e protecao do sistema
            SetFileAttributesA(scriptPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        }
    }

    void Cleanup()
    {
        // 1. Zera a variavel de memoria de ativacao do CLEO ($7500)
        SafeSetCleoGlobalVar(0);

        // Remove residuo do script antigo xxxx.cs (sem trava)
        const char* legacy = "cleo\\xxxx.cs";
        if (GetFileAttributesA(legacy) != INVALID_FILE_ATTRIBUTES)
        {
            SetFileAttributesA(legacy, FILE_ATTRIBUTE_NORMAL);
            DeleteFileA(legacy);
        }

        // Assegura que o novo arquive.cs (com trava) permaneca protegido como oculto e sistema
        const char* scriptPath = "cleo\\arquive.cs";
        if (GetFileAttributesA(scriptPath) != INVALID_FILE_ATTRIBUTES)
        {
            SetFileAttributesA(scriptPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        }

        s_bScriptFound = false;
    }

    static bool SyncScriptThread(bool enable);

    void Initialize()
    {
        AutoDeployIfMissing();
        s_bLastEnabledState = g_MenuState.fastSwitch.enabled;
        s_WasAiming = false;
        s_SwitchState = 0;
        s_StateTimer = 0;
        s_OriginalWeapon = 0;
        s_bInitialized = true;

        // Sincroniza a variavel de memoria compartilhada com o script CLEO ($7500)
        SafeSetCleoGlobalVar(g_MenuState.fastSwitch.enabled ? 1 : 0);

        // Sincroniza a thread CLEO na inicializacao
        s_bScriptFound = SyncScriptThread(g_MenuState.fastSwitch.enabled);

        Logger::Log("[FAST_SWITCH] Modulo FastSwitch (arquive.cs) inicializado. Trava CLEO ($7500): %s",
            g_MenuState.fastSwitch.enabled ? "1 (ON)" : "0 (OFF)");
    }

    void Reset()
    {
        Cleanup();
        s_bScriptFound = false;
        s_bLastEnabledState = false;
        s_WasAiming = false;
        s_SwitchState = 0;
        s_StateTimer = 0;
        s_OriginalWeapon = 0;
    }

    bool IsScriptFound()
    {
        return s_bScriptFound;
    }

    static bool SyncScriptThread(bool enable)
    {
        bool bAnyFound = false;
        __try
        {
            void** ppActiveScripts = reinterpret_cast<void**>(0x00A8B42C);
            if (!ppActiveScripts || !*ppActiveScripts) return false;

            void* pCurrent = *ppActiveScripts;
            while (pCurrent)
            {
                uintptr_t scriptAddr = reinterpret_cast<uintptr_t>(pCurrent);
                bool isMatch = false;

                // 1. IdentificaÃ§Ã£o por nome da thread (CRunningScript::m_szName em +0x08)
                char* szName = reinterpret_cast<char*>(scriptAddr + 0x08);
                if (szName && !IsBadReadPtr(szName, 8))
                {
                    if (_strnicmp(szName, "arquive", 7) == 0 ||
                        _strnicmp(szName, "xxxx", 4) == 0 ||
                        _strnicmp(szName, "fistswi", 7) == 0 ||
                        _strnicmp(szName, "fastswi", 7) == 0 ||
                        _strnicmp(szName, "fist", 4) == 0)
                    {
                        isMatch = true;
                    }
                }

                // 2. Identificacao por assinatura de opcodes (CRunningScript::m_pBaseIP em +0x10)
                if (!isMatch)
                {
                    uint8_t* pBaseIP = *reinterpret_cast<uint8_t**>(scriptAddr + 0x10);
                    if (pBaseIP && !IsBadReadPtr(pBaseIP, 64))
                    {
                        for (int offset = 0; offset <= 32; ++offset)
                        {
                            if (memcmp(pBaseIP + offset, s_Signature10, sizeof(s_Signature10)) == 0)
                            {
                                isMatch = true;
                                break;
                            }
                        }
                    }
                }

                if (isMatch)
                {
                    bAnyFound = true;

                    // Offset 0xC4: m_bIsActive oficial no GTA SA (CRunningScript::Process verifica cmp [ecx+0xC4], 0)
                    *reinterpret_cast<bool*>(scriptAddr + 0xC4) = enable;

                    // Offset 0x38: compatibilidade com variantes do struct
                    *reinterpret_cast<bool*>(scriptAddr + 0x38) = enable;

                    // Offset 0xBC: variÃ¡vel de controle
                    *reinterpret_cast<bool*>(scriptAddr + 0xBC) = enable;

                    // Offset 0xCC: m_nWakeTime do CRunningScript
                    if (!enable)
                    {
                        // Congela a thread CLEO colocando tempo de despertar para o futuro infinito
                        *reinterpret_cast<uint32_t*>(scriptAddr + 0xCC) = 0x7FFFFFFF;
                    }
                    else
                    {
                        // Descongela imediatamente caso estivesse congelada
                        if (*reinterpret_cast<uint32_t*>(scriptAddr + 0xCC) >= 0x70000000)
                        {
                            *reinterpret_cast<uint32_t*>(scriptAddr + 0xCC) = 0;
                        }
                    }
                }

                pCurrent = *reinterpret_cast<void**>(pCurrent);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return bAnyFound;
    }

    void Update()
    {
        if (!s_bInitialized)
            Initialize();

        // 1. Sincroniza variavel de memoria de ativacao para a thread CLEO ($7500)
        SafeSetCleoGlobalVar(g_MenuState.fastSwitch.enabled ? 1 : 0);

        // 2. Sincroniza estado da thread CLEO
        s_bScriptFound = SyncScriptThread(g_MenuState.fastSwitch.enabled);

        if (g_MenuState.fastSwitch.enabled != s_bLastEnabledState)
        {
            s_bLastEnabledState = g_MenuState.fastSwitch.enabled;
            if (g_MenuState.fastSwitch.enabled)
            {
                PlayerSlap::ShowToast("[FastSwitch] ATIVADO (ON)", 0xFF00FF88, 3000);
            }
            else
            {
                PlayerSlap::ShowToast("[FastSwitch] DESATIVADO (OFF)", 0xFFFF4444, 3000);
            }
        }

        if (!g_MenuState.fastSwitch.enabled || g_MenuState.menuOpen)
        {
            s_WasAiming = false;
            s_SwitchState = 0;
            return;
        }

        // 2. Se o script CLEO oficial jÃ¡ estiver ativo no jogo, deixa o CLEO executar nativamente
        if (s_bScriptFound)
        {
            return;
        }

        // 3. ExecuÃ§Ã£o direta 1:1 de arquive.cs sincronizada com o frame
        void* pLocalPed = RuntimeState::GetLocalPed();
        if (!pLocalPed || !RuntimeState::IsPlayerAlive())
        {
            s_WasAiming = false;
            s_SwitchState = 0;
            return;
        }

        if (IsCharDriving(pLocalPed) || IsCharInAir(pLocalPed))
        {
            s_WasAiming = false;
            s_SwitchState = 0;
            return;
        }

        // MÃ¡quina de estados executada a cada frame (respeitando os exatos wait 10 de arquive.cs)
        if (s_SwitchState == 1) // Aguardando 10ms pÃ³s troca para soco
        {
            if (GetTickCount64() - s_StateTimer >= 10)
            {
                pSetCurrentWeapon(pLocalPed, s_OriginalWeapon); // :Noname_181 set_weapon 0@
                s_StateTimer = GetTickCount64();
                s_SwitchState = 2;
            }
            return;
        }
        else if (s_SwitchState == 2) // Aguardando 10ms pÃ³s restauraÃ§Ã£o da arma
        {
            if (GetTickCount64() - s_StateTimer >= 10)
            {
                pSetCurrentWeapon(pLocalPed, 0); // :Noname_181 set_weapon 0
                s_StateTimer = GetTickCount64();
                s_SwitchState = 0; // ConcluÃ­do com Ãªxito
            }
            return;
        }

        bool isAimingNow = ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);

        // Disparo ao soltar a mira (RMB): mirandoAnteriormente and not mirandoAgora
        if (s_WasAiming && !isAimingNow)
        {
            uint32_t currentWeapon = GTA::GetCurrentWeaponId();
            if (currentWeapon != 0) // SÃ³ executa se estiver segurando uma arma de fogo
            {
                s_OriginalWeapon = currentWeapon;
                pSetCurrentWeapon(pLocalPed, 0); // :Noname_119 set_weapon 0
                s_StateTimer = GetTickCount64();
                s_SwitchState = 1;
            }
        }

        s_WasAiming = isAimingNow;
    }
}