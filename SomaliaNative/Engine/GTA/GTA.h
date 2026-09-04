#pragma once
#include <windows.h>
#include <d3d9.h>

#include "../../Render/ImGui/imgui.h"

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

    // Posicao nativa e real da Camera do jogo (0x00B6F99C / TheCamera)
    bool GetCameraPosition(float outPos[3]);

    // Coordenadas reais da mira (Crosshair) na tela do GTA SA 1.0 US
    bool GetCrosshairOffset(float& outX, float& outY);
    ImVec2 GetCrosshairScreenPos();
    float GetMouseSensitivity();
}

