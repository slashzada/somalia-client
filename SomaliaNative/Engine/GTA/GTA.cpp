#include "GTA.h"
#include "../../Core/Logger.h"
#include "../../Core/Main.h"
#include "../../Config/Config.h"
#include <cmath>

namespace GTA
{
    // Endereço único e protegido do dispositivo D3D9 no GTA SA 1.0 US (RwD3D9Device)
    static constexpr uintptr_t ADDR_RW_D3D9_DEVICE = 0x00C97C28;
    // Endereço único da janela Win32 no GTA SA 1.0 US (RsGlobal.ps->window)
    static constexpr uintptr_t ADDR_RW_HWND        = 0x00C97C1C;

    IDirect3DDevice9* GetD3DDevice()
    {
        // Leitura defensiva: verifica se o endereço é acessível e não-nulo
        __try
        {
            IDirect3DDevice9** ppDevice = reinterpret_cast<IDirect3DDevice9**>(ADDR_RW_D3D9_DEVICE);
            if (ppDevice && *ppDevice)
            {
                IDirect3DDevice9* pDevice = *ppDevice;
                // Valida se a VTable é legível
                void** pVTable = *reinterpret_cast<void***>(pDevice);
                if (pVTable && pVTable[17] != nullptr)
                {
                    return pDevice;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }
        return nullptr;
    }

    HWND GetWindowHandle()
    {
        __try
        {
            HWND* pHwnd = reinterpret_cast<HWND*>(ADDR_RW_HWND);
            if (pHwnd && *pHwnd && IsWindow(*pHwnd))
            {
                return *pHwnd;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // fallback
        }

        // Fallback caso a estrutura RenderWare ainda não tenha populado a janela
        HWND hWnd = FindWindowA("Grand theft auto San Andreas", NULL);
        if (hWnd && IsWindow(hWnd))
            return hWnd;

        return GetActiveWindow();
    }

    bool IsReady()
    {
        return (GetD3DDevice() != nullptr) && (GetWindowHandle() != nullptr);
    }

    void* GetPed(int handle)
    {
        if (handle <= 0) return nullptr;
        __try
        {
            auto fnGetPed = reinterpret_cast<void*(__cdecl*)(int)>(0x0054FF90);
            return fnGetPed(handle);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }
    }

    bool GetPedPosition(void* pPed, float outPos[3])
    {
        if (!pPed) return false;
        __try
        {
            uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pPed);
            void* pMatrix = *reinterpret_cast<void**>(pedAddr + 0x14);
            if (pMatrix)
            {
                float* mat = reinterpret_cast<float*>(pMatrix);
                outPos[0] = mat[12];
                outPos[1] = mat[13];
                outPos[2] = mat[14];
                return true;
            }
            float* coords = reinterpret_cast<float*>(pedAddr + 0x4);
            outPos[0] = coords[0];
            outPos[1] = coords[1];
            outPos[2] = coords[2];
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool GetPedBonePosition(void* pPed, int boneId, float outPos[3])
    {
        if (!pPed) return false;
        __try
        {
            auto fnGetBone = reinterpret_cast<void(__thiscall*)(void*, float[3], unsigned int, bool)>(0x005E4280);
            fnGetBone(pPed, outPos, static_cast<unsigned int>(boneId), false);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    float GetPedHealth(void* pPed)
    {
        if (!pPed) return 0.0f;
        __try
        {
            return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(pPed) + 0x540);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0.0f;
        }
    }

    float GetPedArmor(void* pPed)
    {
        if (!pPed) return 0.0f;
        __try
        {
            return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(pPed) + 0x548);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0.0f;
        }
    }

    bool IsPedAlive(void* pPed)
    {
        return (pPed != nullptr) && (GetPedHealth(pPed) > 0.0f);
    }

    uint32_t GetCurrentWeaponId()
    {
        __try
        {
            void* pLocalPed = *reinterpret_cast<void**>(0x00B7CD98);
            if (pLocalPed)
            {
                uint8_t slot = *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(pLocalPed) + 0x718);
                return *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(pLocalPed) + 0x5A0 + slot * 0x1C);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return 0;
    }

    uint16_t GetCameraMode()
    {
        __try
        {
            uint16_t* pMode = reinterpret_cast<uint16_t*>(0x00B6F1A8);
            if (pMode)
                return *pMode;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return 0;
    }

    bool GetCrosshairOffset(float& outX, float& outY)
    {
        uint16_t camMode = GetCameraMode();
        uint32_t weaponId = GetCurrentWeaponId();

        // Armas com luneta / 1ª pessoa (Sniper 34, Rocket Launcher 35, HS Rocket Launcher 36, Camera 43)
        // ou modos de câmera específicos (7: MODE_SNIPER, 8: MODE_ROCKETLAUNCHER, 46: MODE_CAMERA, 51: MODE_ROCKETLAUNCHER_HS)
        // utilizam SEMPRE o centro exato da tela (0.5, 0.5)
        bool isScopedOr1stPerson = (weaponId == 34 || weaponId == 35 || weaponId == 36 || weaponId == 43 ||
                                    camMode == 7 || camMode == 8 || camMode == 46 || camMode == 51);

        // O GTA San Andreas só ativa a mira sobre o ombro (3rd person) nos modos 53 (MODE_AIMWEAPON) e 55 (MODE_SPECIAL_AIM).
        // Se o jogador estiver com arma de luneta ou NÃO estiver mirando em 3ª pessoa,
        // a mira e o centro da tela são exatamente (0.5, 0.5).
        if (!isScopedOr1stPerson && (camMode == 53 || camMode == 55))
        {
            __try
            {
                float* pCrossX = reinterpret_cast<float*>(0x00B6EC14);
                float* pCrossY = reinterpret_cast<float*>(0x00B6EC10);
                if (pCrossX && pCrossY)
                {
                    float x = *pCrossX;
                    float y = *pCrossY;
                    if (x > 0.05f && x < 0.95f && y > 0.05f && y < 0.95f)
                    {
                        outX = x;
                        outY = y;
                        return true;
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        outX = 0.5f;
        outY = 0.5f;
        return false;
    }

    ImVec2 GetCrosshairScreenPos()
    {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        if (displaySize.x <= 0.0f || displaySize.y <= 0.0f)
        {
            __try
            {
                DWORD* pWidth  = reinterpret_cast<DWORD*>(0x00C17044);
                DWORD* pHeight = reinterpret_cast<DWORD*>(0x00C17048);
                if (pWidth && pHeight && *pWidth > 0 && *pHeight > 0)
                {
                    displaySize = ImVec2(static_cast<float>(*pWidth), static_cast<float>(*pHeight));
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        if (displaySize.x <= 0.0f || displaySize.y <= 0.0f)
            return ImVec2(0.0f, 0.0f);

        float ox = 0.5f, oy = 0.5f;
        GetCrosshairOffset(ox, oy);
        return ImVec2(displaySize.x * ox, displaySize.y * oy);
    }

    float GetMouseSensitivity()
    {
        __try
        {
            float* pSens = reinterpret_cast<float*>(0x00B6EC1C);
            if (pSens && *pSens > 0.0001f && *pSens < 10.0f)
            {
                return *pSens;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        return 0.0025f;
    }

    bool GetCameraFront(float outFront[3])
    {
        __try
        {
            uint8_t* pActiveCam = reinterpret_cast<uint8_t*>(0x00B6F081);
            if (!pActiveCam) return false;
            uint8_t activeCam = *pActiveCam;
            if (activeCam > 2) activeCam = 0;

            uintptr_t pCam = 0x00B6F1A8 + static_cast<uintptr_t>(activeCam) * 0x238;
            float* pFront = reinterpret_cast<float*>(pCam + 0x184);
            if (pFront && !IsBadReadPtr(pFront, sizeof(float) * 3))
            {
                outFront[0] = pFront[0];
                outFront[1] = pFront[1];
                outFront[2] = pFront[2];
                return true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return false;
    }

    bool SetCameraFront(const float inFront[3])
    {
        __try
        {
            uint8_t* pActiveCam = reinterpret_cast<uint8_t*>(0x00B6F081);
            if (!pActiveCam) return false;
            uint8_t activeCam = *pActiveCam;
            if (activeCam > 2) activeCam = 0;

            uintptr_t pCam = 0x00B6F1A8 + static_cast<uintptr_t>(activeCam) * 0x238;
            float* pFront = reinterpret_cast<float*>(pCam + 0x184);
            if (pFront && !IsBadWritePtr(pFront, sizeof(float) * 3))
            {
                pFront[0] = inFront[0];
                pFront[1] = inFront[1];
                pFront[2] = inFront[2];
                return true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return false;
    }

    bool GetCameraSource(float outSource[3])
    {
        __try
        {
            uint8_t* pActiveCam = reinterpret_cast<uint8_t*>(0x00B6F081);
            if (!pActiveCam) return false;
            uint8_t activeCam = *pActiveCam;
            if (activeCam > 2) activeCam = 0;

            uintptr_t pCam = 0x00B6F1A8 + static_cast<uintptr_t>(activeCam) * 0x238;
            float* pSource = reinterpret_cast<float*>(pCam + 0x190);
            if (pSource && !IsBadReadPtr(pSource, sizeof(float) * 3))
            {
                outSource[0] = pSource[0];
                outSource[1] = pSource[1];
                outSource[2] = pSource[2];
                return true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return false;
    }

    static WeaponFirePreHandler_t s_WeaponFirePreHandler = nullptr;

    void SetWeaponFirePreHandler(WeaponFirePreHandler_t handler)
    {
        s_WeaponFirePreHandler = handler;
    }

    typedef bool(__thiscall* FireInstantHit_t)(void* pWeapon, void* pPed, void* pOrigin, void* pTarget, void* pTargetEntity, int arg5, int arg6);
    static FireInstantHit_t s_OriginalFireInstantHit = nullptr;
    static unsigned char s_TrampolineFireInstantHit[32] = { 0 };
    static unsigned char s_OriginalFireBytes[6] = { 0 };
    static bool s_WeaponHookInstalled = false;

    static bool __fastcall Hooked_FireInstantHit(void* pWeapon, void* edx, void* pPed, void* pOrigin, void* pTarget, void* pTargetEntity, int arg5, int arg6)
    {
        Main::CallbackGuard guard;
        if (!s_OriginalFireInstantHit) return false;
        if (!guard.IsActive()) return s_OriginalFireInstantHit(pWeapon, pPed, pOrigin, pTarget, pTargetEntity, arg5, arg6);

        float savedFront[3] = { 0.0f, 0.0f, 0.0f };
        float savedTarget[3] = { 0.0f, 0.0f, 0.0f };
        bool modifiedTarget = false;
        bool redirected = false;

        if (s_WeaponFirePreHandler)
        {
            redirected = s_WeaponFirePreHandler(pWeapon, pPed, pOrigin, pTarget, savedFront, savedTarget, modifiedTarget);
        }

        bool result = s_OriginalFireInstantHit(pWeapon, pPed, pOrigin, pTarget, pTargetEntity, arg5, arg6);

        // Restaura imediatamente os vetores após a execução do disparo nativo
        // Garantindo que a mira do jogador NÃO se mova nem trema na tela
        if (redirected)
        {
            SetCameraFront(savedFront);
            if (modifiedTarget && pTarget && !IsBadWritePtr(pTarget, sizeof(float) * 3))
            {
                float* pTargetVec = reinterpret_cast<float*>(pTarget);
                pTargetVec[0] = savedTarget[0];
                pTargetVec[1] = savedTarget[1];
                pTargetVec[2] = savedTarget[2];
            }
        }

        return result;
    }

    bool InstallWeaponHooks()
    {
        if (s_WeaponHookInstalled) return true;
        if (Main::IsShuttingDown()) return false;

        uintptr_t target = 0x00742300;
        if (IsBadReadPtr(reinterpret_cast<void*>(target), 6)) return false;

        // Salva os 6 bytes originais (83 ec 3c 53 56 57)
        memcpy(s_OriginalFireBytes, reinterpret_cast<void*>(target), 6);

        // Prepara o trampoline
        DWORD oldProtect = 0;
        if (!VirtualProtect(s_TrampolineFireInstantHit, sizeof(s_TrampolineFireInstantHit), PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            return false;
        }

        // Copia os 6 bytes originais para o trampoline
        memcpy(s_TrampolineFireInstantHit, reinterpret_cast<void*>(target), 6);

        // Adiciona JMP de volta para target + 6
        s_TrampolineFireInstantHit[6] = 0xE9;
        uintptr_t trampJumpFrom = reinterpret_cast<uintptr_t>(&s_TrampolineFireInstantHit[6]);
        uintptr_t trampJumpTo = target + 6;
        *reinterpret_cast<int32_t*>(&s_TrampolineFireInstantHit[7]) = static_cast<int32_t>(trampJumpTo - (trampJumpFrom + 5));
        s_OriginalFireInstantHit = reinterpret_cast<FireInstantHit_t>(static_cast<void*>(s_TrampolineFireInstantHit));

        // Instala o detour hook no target (E9 <rel32> 90)
        DWORD targetOldProtect = 0;
        if (VirtualProtect(reinterpret_cast<void*>(target), 6, PAGE_EXECUTE_READWRITE, &targetOldProtect))
        {
            unsigned char* pTargetBytes = reinterpret_cast<unsigned char*>(target);
            pTargetBytes[0] = 0xE9;
            uintptr_t hookAddr = reinterpret_cast<uintptr_t>(&Hooked_FireInstantHit);
            *reinterpret_cast<int32_t*>(&pTargetBytes[1]) = static_cast<int32_t>(hookAddr - (target + 5));
            pTargetBytes[5] = 0x90; // NOP

            VirtualProtect(reinterpret_cast<void*>(target), 6, targetOldProtect, &targetOldProtect);
            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(target), 6);

            s_WeaponHookInstalled = true;
            Logger::Log("[GTA][HOOK] CWeapon::FireInstantHit (0x%p) detour hook instalado com sucesso!", reinterpret_cast<void*>(target));
            return true;
        }

        return false;
    }

    void UninstallWeaponHooks()
    {
        if (!s_WeaponHookInstalled) return;

        uintptr_t target = 0x00742300;
        DWORD oldProtect = 0;
        if (VirtualProtect(reinterpret_cast<void*>(target), 6, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            memcpy(reinterpret_cast<void*>(target), s_OriginalFireBytes, 6);
            VirtualProtect(reinterpret_cast<void*>(target), 6, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(target), 6);
            Logger::Log("[GTA][UNHOOK] CWeapon::FireInstantHit hook desinstalado com sucesso.");
        }

        s_WeaponHookInstalled = false;
        s_OriginalFireInstantHit = nullptr;
    }

    bool IsWeaponHooked()
    {
        return s_WeaponHookInstalled;
    }
}


