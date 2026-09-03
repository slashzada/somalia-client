#include "GTA.h"

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
            fnGetBone(pPed, outPos, static_cast<unsigned int>(boneId), true);
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
}
