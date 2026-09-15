#pragma once
#include <windows.h>
#include <stdint.h>
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

    // Coordenadas reais da mira (Crosshair) na tela do GTA SA 1.0 US
    uint16_t GetCameraMode();
    uint32_t GetCurrentWeaponId();
    bool GetCrosshairOffset(float& outX, float& outY);
    ImVec2 GetCrosshairScreenPos();
    float GetMouseSensitivity();

    // Controle e redirecionamento de câmera do motor GTA SA (Silent Aim)
    bool GetCameraFront(float outFront[3]);
    bool SetCameraFront(const float inFront[3]);
    bool GetCameraSource(float outSource[3]);

    // Callback pré-disparo para redirecionamento nativo em CWeapon::FireInstantHit (0x00742300)
    typedef bool (*WeaponFirePreHandler_t)(void* pWeapon, void* pPed, void* pOrigin, void* pTarget,
                                           float outSavedFront[3], float outSavedTarget[3], bool& outModifiedTarget);
    void SetWeaponFirePreHandler(WeaponFirePreHandler_t handler);

    // Detour hook nativo em CWeapon::FireInstantHit (0x00742300)
    bool InstallWeaponHooks();
    void UninstallWeaponHooks();
    bool IsWeaponHooked();
}

