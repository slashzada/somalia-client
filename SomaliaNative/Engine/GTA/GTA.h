#pragma once
#include <windows.h>
#include <d3d9.h>

namespace GTA
{
    // Obtenção segura e isolada do dispositivo Direct3D 9 do RenderWare (0x00C97C28)
    IDirect3DDevice9* GetD3DDevice();

    // Obtenção da janela Win32 do GTA San Andreas (0x00C97C1C)
    HWND GetWindowHandle();

    // Verificação de prontidão do motor gráfico
    bool IsReady();

    // Funções e estruturas nativas do GTA San Andreas 1.0 US
    void* GetPed(int handle);
    bool GetPedPosition(void* pPed, float outPos[3]);
    bool GetPedBonePosition(void* pPed, int boneId, float outPos[3]);
    float GetPedHealth(void* pPed);
    float GetPedArmor(void* pPed);
    bool IsPedAlive(void* pPed);
}
