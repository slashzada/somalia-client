#include "Aimbot.h"
#include "AimAssist.h"
#include "RageBot.h"
#include "../../Core/Logger.h"
#include "../../Core/RuntimeState.h"
#include "../../Render/ImGui/imgui.h"
#include "../../Engine/SAMP/SAMP.h"
#include "../../Engine/GTA/GTA.h"
#include <stdio.h>

namespace Aimbot
{
    static TargetInfo s_CurrentTarget = {};
    static uint64_t s_LastLogTick = 0;
    static int s_LastLoggedTargetId = -1;

    float GetFovRadius(float fovPercent)
    {
        // 100% de FOV equivale a 400 pixels na resolução de tela (escala sincronizada com o círculo do ESP)
        return fovPercent * 4.0f;
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

    const char* GetWeaponProfileName(int group)
    {
        switch (group)
        {
        case 0: return "Snipers";
        case 1: return "Pistols";
        case 2: return "Rifles";
        case 3: return "Shotguns";
        default: return "Default";
        }
    }

    int GetActiveWeaponGroup()
    {
        __try
        {
            void* pLocalPed = *reinterpret_cast<void**>(0x00B7CD98);
            if (pLocalPed)
            {
                uint8_t slot = *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(pLocalPed) + 0x718);
                uint32_t weaponType = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(pLocalPed) + 0x5A0 + slot * 0x1C);

                // Snipers: 33 (Country Rifle), 34 (Sniper Rifle) -> Grupo 0
                if (weaponType == 33 || weaponType == 34)
                    return 0;

                // Pistols: 22 (Colt 45), 23 (Silenced), 24 (Desert Eagle) -> Grupo 1
                if (weaponType >= 22 && weaponType <= 24)
                    return 1;

                // Rifles / SMGs: 28 (Uzi), 29 (MP5), 30 (AK47), 31 (M4), 32 (Tec9) -> Grupo 2
                if (weaponType >= 28 && weaponType <= 32)
                    return 2;

                // Shotguns: 25 (Shotgun), 26 (Sawnoff), 27 (Combat Shotgun) -> Grupo 3
                if (weaponType >= 25 && weaponType <= 27)
                    return 3;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }

        // Fallback para a arma atualmente selecionada no menu
        return g_MenuState.aimbot.autoSnipersType;
    }

    WeaponAimConfig& GetCurrentWeaponProfile()
    {
        // Quando o menu estiver aberto, edita a arma selecionada na interface
        if (g_MenuState.menuOpen)
        {
            int selected = g_MenuState.aimbot.autoSnipersType;
            if (selected < 0 || selected >= 4)
                selected = 0;
            return g_MenuState.aimbot.weapons[selected];
        }

        int activeGroup = GetActiveWeaponGroup();
        if (activeGroup < 0 || activeGroup >= 4)
            activeGroup = 0;

        return g_MenuState.aimbot.weapons[activeGroup];
    }

    const TargetInfo& GetCurrentTarget()
    {
        return s_CurrentTarget;
    }

    void ClearTarget()
    {
        s_CurrentTarget = {};
        s_CurrentTarget.valid = false;
        s_LastLoggedTargetId = -1;
        AimAssist::Reset();
    }

    void Update()
    {
        if (!RuntimeState::IsPlayerAlive())
        {
            ClearTarget();
            return;
        }

        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        if (displaySize.x <= 0 || displaySize.y <= 0)
        {
            s_CurrentTarget.valid = false;
            return;
        }

        ImVec2 screenCenter = GTA::GetCrosshairScreenPos();

        // Se o Aimbot e Silent Aim estiverem desligados globalmente, limpa o alvo
        if (!g_MenuState.aimbot.enabled && !g_MenuState.silentAim.enabled)
        {
            if (s_CurrentTarget.valid)
            {
                Logger::Log("[AIMBOT] Target changed: old=%d new=-1", s_CurrentTarget.playerId);
            }
            s_CurrentTarget.valid = false;
            return;
        }

        uint32_t weaponId = GetCurrentWeaponId();
        if (weaponId < 22 || weaponId > 34)
        {
            s_CurrentTarget.valid = false;
            s_LastLoggedTargetId = -1;
            return;
        }

        WeaponAimConfig profile = GetCurrentWeaponProfile();
        int activeGroup = GetActiveWeaponGroup();
        if (activeGroup < 0 || activeGroup >= 4) activeGroup = 0;

        bool isSilentActive = false;
        if (g_MenuState.silentAim.enabled)
        {
            bool hookOk = SAMP::EnsureRakHook();
            static uint64_t s_lastHookDiagTick = 0;
            uint64_t nowTick = GetTickCount64();
            if (nowTick - s_lastHookDiagTick >= 3000)
            {
                Logger::Log("[AIMBOT][DIAG] EnsureRakHook() invocado -> retorno=%s | isHooked=%d",
                    hookOk ? "TRUE" : "FALSE", SAMP::IsRakHooked() ? 1 : 0);
                s_lastHookDiagTick = nowTick;
            }

            const auto& sw = g_MenuState.silentAim.weapons[activeGroup];
            if (sw.enabled)
            {
                profile.fov = sw.fov;
                profile.maxDistance = sw.maxDistance;
                profile.priority = sw.priority;
                profile.ignoreDead = sw.ignoreDead;
                profile.teamCheck = sw.teamCheck;
                profile.visibilityCheck = sw.visibilityCheck;
                profile.enabled = true;

                // Bone RANDOM (4): resolve para um osso real no frame atual
                // (No momento do disparo, o SAMP.cpp resolve de novo independentemente)
                if (sw.bone == 4)
                {
                    static int s_lastRandomBoneFrame = 0;
                    static int s_cachedRandomBone = 0;
                    // Recalcula o random a cada 300ms para evitar oscilação visual
                    uint64_t now = GetTickCount64();
                    if (s_lastRandomBoneFrame == 0 || (now - (uint64_t)s_lastRandomBoneFrame) > 300)
                    {
                        int options[] = { 0, 1, 2, 3 }; // HEAD, NECK, CHEST, PELVIS
                        s_cachedRandomBone = options[rand() % 4];
                        s_lastRandomBoneFrame = (int)now;
                    }
                    profile.bone = s_cachedRandomBone;
                }
                else
                {
                    profile.bone = sw.bone;
                }

                isSilentActive = true;
            }
        }

        float fovRadius = GetFovRadius(profile.fov);

        int candidates = 0;
        int insideFov = 0;

        TargetInfo newTarget = TargetSelector::FindBestTarget(profile, screenCenter, fovRadius, candidates, insideFov, false);

        // Registro de mudança de alvo somente quando houver alteração
        int newTargetId = newTarget.valid ? newTarget.playerId : -1;
        if (newTargetId != s_LastLoggedTargetId)
        {
            Logger::Log("[AIMBOT] Target changed: old=%d new=%d", s_LastLoggedTargetId, newTargetId);
            s_LastLoggedTargetId = newTargetId;
        }

        s_CurrentTarget = newTarget;

        // Telemetria por throttling (~1 segundo)
        uint64_t currentTick = GetTickCount64();
        if (currentTick - s_LastLogTick >= 1000)
        {
            if (s_CurrentTarget.valid)
            {
                Logger::Log("[AIMBOT] candidates=%d inside_fov=%d selected_id=%d distance=%.1f screen_distance=%.1f bone=%s",
                    candidates, insideFov, s_CurrentTarget.playerId, s_CurrentTarget.distance3D, s_CurrentTarget.distanceFromCrosshair, s_CurrentTarget.boneName);
            }
            else
            {
                Logger::Log("[AIMBOT] candidates=%d inside_fov=%d selected_id=-1 distance=0.0 screen_distance=0.0 bone=NONE",
                    candidates, insideFov);
            }
            s_LastLogTick = currentTick;
        }
    }

    void Render()
    {
        // 1. Processa a seleção de alvos
        Update();

        // 2. Se o Aimbot estiver ativo, sincroniza o raio visual do círculo FOV
        WeaponAimConfig& profile = GetCurrentWeaponProfile();
        if (g_MenuState.aimbot.enabled)
        {
            g_MenuState.visuals.fovCircleRadius = static_cast<int>(profile.fov);
        }

        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        if (displaySize.x <= 0 || displaySize.y <= 0) return;
        ImVec2 screenCenter = GTA::GetCrosshairScreenPos();

        // 3. Processa e Renderiza o Aim Assist (Cálculo suave e vetor de diagnóstico)
        AimAssist::Process(s_CurrentTarget, profile, screenCenter);
        if (!g_MenuState.silentAim.enabled && g_MenuState.legitBot.enabled)
        {
            AimAssist::RenderDebugVisuals(screenCenter, profile);
        }

        // 4. Renderiza o Target Indicator na tela
        if (g_MenuState.aimbot.enabled && s_CurrentTarget.valid && profile.drawTargetMarker)
        {
            ImDrawList* draw = ImGui::GetForegroundDrawList();
            if (draw)
            {
                ImVec2 pos = s_CurrentTarget.screenPosition;

                // Anel de mira e ponto central no osso visado
                draw->AddCircle(pos, 7.5f, IM_COL32(255, 60, 60, 240), 20, 1.8f);
                draw->AddCircleFilled(pos, 2.5f, IM_COL32(255, 255, 255, 255));

                // Colchetes laterais [ O ]
                draw->AddLine(ImVec2(pos.x - 13, pos.y), ImVec2(pos.x - 9, pos.y), IM_COL32(255, 60, 60, 220), 1.5f);
                draw->AddLine(ImVec2(pos.x + 9, pos.y), ImVec2(pos.x + 13, pos.y), IM_COL32(255, 60, 60, 220), 1.5f);

                // Tag de texto com ID, Bone e Distância
                char tag[64];
                snprintf(tag, sizeof(tag), "[%d] %s (%.1fm)", s_CurrentTarget.playerId, s_CurrentTarget.boneName, s_CurrentTarget.distance3D);
                draw->AddText(ImVec2(pos.x + 15, pos.y - 8), IM_COL32(0, 0, 0, 255), tag);
                draw->AddText(ImVec2(pos.x + 14, pos.y - 9), IM_COL32(255, 80, 80, 255), tag);

                // Linha traçante opcional conectando centro de mira ao alvo
                if (profile.drawTracer)
                {
                    draw->AddLine(screenCenter, pos, IM_COL32(255, 60, 60, 140), 1.0f);
                }
            }
        }
    }
}
