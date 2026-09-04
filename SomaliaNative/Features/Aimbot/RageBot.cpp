#include "RageBot.h"
#include "TargetSelector.h"
#include "../../Core/Logger.h"
#include "../../Core/RuntimeState.h"
#include "../../Engine/GTA/GTA.h"
#include <math.h>
#include <stdio.h>

namespace RageBot
{
    static TargetInfo   s_RageTarget = {};
    static RageBotState s_RageState  = {};
    static uint64_t     s_LastLogTick = 0;
    static int          s_LastLoggedTargetId = -1;
    static float        s_RageAccumulatedX = 0.0f;
    static float        s_RageAccumulatedY = 0.0f;

    static bool ApplyRageAim(float outX, float outY)
    {
        s_RageAccumulatedX += outX;
        s_RageAccumulatedY += outY;

        int moveX = static_cast<int>(s_RageAccumulatedX);
        int moveY = static_cast<int>(s_RageAccumulatedY);

        if (moveX == 0 && moveY == 0)
            return false;

        s_RageAccumulatedX -= static_cast<float>(moveX);
        s_RageAccumulatedY -= static_cast<float>(moveY);

        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        input.mi.dx = moveX;
        input.mi.dy = moveY;
        input.mi.dwExtraInfo = 0;
        input.mi.time = 0;
        SendInput(1, &input, sizeof(INPUT));
        return true;
    }

    float GetFovRadius(float fovPercent)
    {
        // 100% de FOV equivale a 400 pixels na resolução de tela
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
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
        return 0;
    }

    int GetActiveWeaponGroup()
    {
        uint32_t weaponType = GetCurrentWeaponId();

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

        // Fallback para o grupo configurado na interface do Ragebot
        return g_MenuState.rageBot.currentWeaponGroup;
    }

    const char* GetWeaponProfileTag(int group)
    {
        switch (group)
        {
        case 0: return "RAGE_SNIPER";
        case 1: return "RAGE_PISTOL";
        case 2: return "RAGE_RIFLE";
        case 3: return "RAGE_SHOTGUN";
        default: return "RAGE_DEFAULT";
        }
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

    RageWeaponConfig& GetCurrentWeaponProfile()
    {
        // Se o menu estiver aberto, usa o grupo atualmente selecionado na interface Rage
        if (g_MenuState.menuOpen)
        {
            int selected = g_MenuState.rageBot.currentWeaponGroup;
            if (selected < 0 || selected >= 4)
                selected = 0;
            return g_MenuState.rageBot.weapons[selected];
        }

        int activeGroup = GetActiveWeaponGroup();
        if (activeGroup < 0 || activeGroup >= 4)
            activeGroup = 0;

        return g_MenuState.rageBot.weapons[activeGroup];
    }

    const TargetInfo& GetCurrentTarget()
    {
        return s_RageTarget;
    }

    const RageBotState& GetState()
    {
        return s_RageState;
    }

    bool CheckActivationCondition(int activationMode)
    {
        // Se o menu estiver aberto, Ragebot permanece inativo para não atrapalhar cliques na UI
        if (g_MenuState.menuOpen)
            return false;

        switch (activationMode)
        {
        case 0: // Always
            return true;

        case 1: // While Aiming (Botão Direito do Mouse - RMB)
            return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

        case 2: // While Shooting (Botão Esquerdo do Mouse - LMB)
            return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        case 3: // While Aiming + Shooting (Ambos pressionados)
            return ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0) &&
                   ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);

        default:
            return false;
        }
    }

    void Reset()
    {
        s_RageTarget.valid = false;
        s_RageState.isActive = false;
        s_RageState.enabledPass = false;
        s_RageState.activationPass = false;
        s_RageState.fovPass = false;
        s_RageState.targetId = -1;
        s_RageAccumulatedX = 0.0f;
        s_RageAccumulatedY = 0.0f;
        snprintf(s_RageState.boneName, sizeof(s_RageState.boneName), "HEAD");
        snprintf(s_RageState.statusString, sizeof(s_RageState.statusString), "STANDBY");
        s_RageState.deltaX = 0.0f;
        s_RageState.deltaY = 0.0f;
        s_RageState.stepX = 0.0f;
        s_RageState.stepY = 0.0f;
        s_RageState.aggressiveness = 100.0f;
        s_RageState.targetScreen = ImVec2(0, 0);
        s_RageState.screenCenter = ImVec2(0, 0);
    }

    void Update()
    {
        if (!RuntimeState::IsPlayerAlive())
        {
            Reset();
            snprintf(s_RageState.statusString, sizeof(s_RageState.statusString), "DEAD");
            return;
        }

        uint64_t currentTick = GetTickCount64();
        bool shouldLog = (currentTick - s_LastLogTick >= 1000);

        // ─────────────────────────────────────────────────────────────
        // ETAPA 1: RageBot enabled
        // ─────────────────────────────────────────────────────────────
        if (!g_MenuState.rageBot.enabled)
        {
            Reset();
            snprintf(s_RageState.statusString, sizeof(s_RageState.statusString), "DISABLED");
            if (shouldLog)
            {
                Logger::Log("[SOMALIA][RAGE] FAIL: enabled");
                s_LastLogTick = currentTick;
            }
            return;
        }
        s_RageState.enabledPass = true;

        // Se o menu estiver aberto na tela, permanece em standby silencioso
        if (g_MenuState.menuOpen)
        {
            s_RageState.isActive = false;
            s_RageState.stepX = 0.0f;
            s_RageState.stepY = 0.0f;
            snprintf(s_RageState.statusString, sizeof(s_RageState.statusString), "STANDBY (menu open)");
            return;
        }

        // ─────────────────────────────────────────────────────────────
        // ETAPA 2: Weapon Profile & Identificação de Arma
        // ─────────────────────────────────────────────────────────────
        int activeGroup = GetActiveWeaponGroup();
        uint32_t weaponId = GetCurrentWeaponId();

        // Se o jogador estiver desarmado (punho, faca, taco, granada, etc), Ragebot não ativa
        if (weaponId < 22 || weaponId > 34)
        {
            s_RageState.isActive = false;
            s_RageState.targetId = -1;
            s_RageState.stepX = 0.0f;
            s_RageState.stepY = 0.0f;
            s_RageTarget.valid = false;
            snprintf(s_RageState.statusString, sizeof(s_RageState.statusString), "STANDBY (no firearm)");
            return;
        }

        RageWeaponConfig& profile = GetCurrentWeaponProfile();
        const char* profileTag = GetWeaponProfileTag(activeGroup);

        if (!profile.enabled)
        {
            s_RageState.isActive = false;
            s_RageState.stepX = 0.0f;
            s_RageState.stepY = 0.0f;
            snprintf(s_RageState.statusString, sizeof(s_RageState.statusString), "FAIL: profile disabled");
            if (shouldLog)
            {
                Logger::Log("[SOMALIA][RAGE] FAIL: weapon_profile (weapon=%u profile=%s enabled=0)", weaponId, profileTag);
                s_LastLogTick = currentTick;
            }
            return;
        }

        // ─────────────────────────────────────────────────────────────
        // ETAPA 3: Activation Condition
        // ─────────────────────────────────────────────────────────────
        bool activationMet = CheckActivationCondition(profile.activationMode);
        s_RageState.activationPass = activationMet;
        if (!activationMet)
        {
            s_RageState.isActive = false;
            s_RageState.stepX = 0.0f;
            s_RageState.stepY = 0.0f;
            snprintf(s_RageState.statusString, sizeof(s_RageState.statusString), "FAIL: activation condition");
            if (shouldLog)
            {
                Logger::Log("[SOMALIA][RAGE] FAIL: activation (mode=%d)", profile.activationMode);
                s_LastLogTick = currentTick;
            }
            return;
        }

        // ─────────────────────────────────────────────────────────────
        // ETAPA 4: Target Selection
        // ─────────────────────────────────────────────────────────────
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        if (displaySize.x <= 0 || displaySize.y <= 0)
        {
            Reset();
            return;
        }
        ImVec2 screenCenter = GTA::GetCrosshairScreenPos();

        WeaponAimConfig selectorConfig;
        selectorConfig.bone = profile.bone; // Padrão: 0 (HEAD -> Osso 8)
        selectorConfig.maxDistance = profile.maxDistance;
        selectorConfig.priority = profile.priority;
        selectorConfig.ignoreDead = profile.ignoreDead;
        selectorConfig.teamCheck = profile.teamCheck;
        selectorConfig.visibilityCheck = profile.visibilityCheck;

        float fovRadius = GetFovRadius(profile.fov);
        int candidates = 0;
        int insideFovCount = 0;

        TargetInfo newTarget = TargetSelector::FindBestTarget(selectorConfig, screenCenter, fovRadius, candidates, insideFovCount, true);

        int newTargetId = newTarget.valid ? newTarget.playerId : -1;
        if (newTargetId != s_LastLoggedTargetId)
        {
            s_RageAccumulatedX = 0.0f;
            s_RageAccumulatedY = 0.0f;
            Logger::Log("[SOMALIA][RAGE] Target changed: old=%d new=%d", s_LastLoggedTargetId, newTargetId);
            s_LastLoggedTargetId = newTargetId;
        }

        s_RageTarget = newTarget;

        // ─────────────────────────────────────────────────────────────
        // ETAPA 5: Target Valid & FOV Valid
        // ─────────────────────────────────────────────────────────────
        if (!s_RageTarget.valid)
        {
            s_RageState.isActive = false;
            s_RageState.targetId = -1;
            s_RageState.fovPass = false;
            s_RageState.stepX = 0.0f;
            s_RageState.stepY = 0.0f;

            if (candidates == 0)
                snprintf(s_RageState.statusString, sizeof(s_RageState.statusString), "SEARCHING (no targets)");
            else if (insideFovCount == 0)
                snprintf(s_RageState.statusString, sizeof(s_RageState.statusString), "FAIL: out of FOV");
            else
                snprintf(s_RageState.statusString, sizeof(s_RageState.statusString), "FAIL: target invalid");

            if (shouldLog)
            {
                if (candidates == 0)
                {
                    Logger::Log("[SOMALIA][RAGE] FAIL: target (candidates=0)");
                }
                else if (insideFovCount == 0)
                {
                    Logger::Log("[SOMALIA][RAGE] FAIL: fov (candidates=%d insideFov=0 radius=%.1f)", candidates, fovRadius);
                }
                else
                {
                    Logger::Log("[SOMALIA][RAGE] FAIL: target");
                }
                s_LastLogTick = currentTick;
            }
            return;
        }
        s_RageState.fovPass = true;

        // ─────────────────────────────────────────────────────────────
        // ETAPA 6: Bone Valid & Screen Position Valid
        // ─────────────────────────────────────────────────────────────
        if (isnan(s_RageTarget.screenPosition.x) || isinf(s_RageTarget.screenPosition.x) ||
            isnan(s_RageTarget.screenPosition.y) || isinf(s_RageTarget.screenPosition.y) ||
            (s_RageTarget.screenPosition.x == 0.0f && s_RageTarget.screenPosition.y == 0.0f))
        {
            s_RageState.isActive = false;
            snprintf(s_RageState.statusString, sizeof(s_RageState.statusString), "FAIL: bone invalid");
            if (shouldLog)
            {
                Logger::Log("[SOMALIA][RAGE] FAIL: bone");
                s_LastLogTick = currentTick;
            }
            return;
        }

        // ─────────────────────────────────────────────────────────────
        // ETAPA 7: Delta X/Y Calculation
        // ─────────────────────────────────────────────────────────────
        float deltaX = s_RageTarget.screenPosition.x - screenCenter.x;
        float deltaY = s_RageTarget.screenPosition.y - screenCenter.y;
        if (isnan(deltaX) || isinf(deltaX) || isnan(deltaY) || isinf(deltaY))
        {
            s_RageState.isActive = false;
            snprintf(s_RageState.statusString, sizeof(s_RageState.statusString), "FAIL: delta invalid");
            if (shouldLog)
            {
                Logger::Log("[SOMALIA][RAGE] FAIL: delta");
                s_LastLogTick = currentTick;
            }
            return;
        }

        // ─────────────────────────────────────────────────────────────
        // ETAPA 8: Aggressiveness & Output Calculation
        // ─────────────────────────────────────────────────────────────
        float aggr = profile.aggressiveness;
        if (aggr < 0.0f) aggr = 0.0f;
        if (aggr > 100.0f) aggr = 100.0f;

        float aggrFactor = aggr / 100.0f;
        float sensRatio = 0.20f;
        float outputX = deltaX * sensRatio * aggrFactor;
        float outputY = deltaY * sensRatio * aggrFactor;

        // Limite máximo de passo por quadro para estabilidade absoluta (previne arremesso fora de quadro)
        float maxStep = 24.0f;
        if (outputX > maxStep) outputX = maxStep;
        else if (outputX < -maxStep) outputX = -maxStep;

        if (outputY > maxStep) outputY = maxStep;
        else if (outputY < -maxStep) outputY = -maxStep;

        if (aggr > 0.0f && outputX == 0.0f && outputY == 0.0f && (deltaX != 0.0f || deltaY != 0.0f))
        {
            if (shouldLog)
            {
                Logger::Log("[SOMALIA][RAGE] FAIL: output");
                s_LastLogTick = currentTick;
            }
            return;
        }

        s_RageState.isActive = true;
        s_RageState.targetId = s_RageTarget.playerId;
        snprintf(s_RageState.boneName, sizeof(s_RageState.boneName), "%s", s_RageTarget.boneName ? s_RageTarget.boneName : "HEAD");
        s_RageState.deltaX = deltaX;
        s_RageState.deltaY = deltaY;
        s_RageState.stepX = outputX;
        s_RageState.stepY = outputY;
        s_RageState.aggressiveness = aggr;
        s_RageState.targetScreen = s_RageTarget.screenPosition;
        s_RageState.screenCenter = screenCenter;

        // ─────────────────────────────────────────────────────────────
        // ETAPA 8.1: Atuação Física de Mira (SendInput Imediato)
        // ─────────────────────────────────────────────────────────────
        bool applied = ApplyRageAim(outputX, outputY);
        if (applied)
        {
            snprintf(s_RageState.statusString, sizeof(s_RageState.statusString), "APPLIED (AIM ACTIVE)");
        }
        else
        {
            snprintf(s_RageState.statusString, sizeof(s_RageState.statusString), "OUTPUT GENERATED");
        }

        // ─────────────────────────────────────────────────────────────
        // ETAPA 9: Log Obrigatório PASS
        // ─────────────────────────────────────────────────────────────
        if (shouldLog)
        {
            const char* boneName = s_RageTarget.boneName ? s_RageTarget.boneName : "HEAD";
            Logger::Log("[SOMALIA][RAGE] enabled=1 activation=1 weapon=%u profile=%s target=%d targetValid=1 bone=%s fov=%.0f insideFov=1 screen=(%.1f,%.1f) delta=(%.1f,%.1f) aggressiveness=%.0f output=(%.1f,%.1f)",
                weaponId, profileTag, s_RageTarget.playerId, boneName,
                profile.fov,
                s_RageTarget.screenPosition.x, s_RageTarget.screenPosition.y,
                deltaX, deltaY, aggr, outputX, outputY);
            s_LastLogTick = currentTick;
        }
    }

    void Render()
    {
        // Processa o ciclo de atualização e auditoria do pipeline
        Update();

        if (!g_MenuState.rageBot.enabled)
            return;

        RageWeaponConfig& profile = GetCurrentWeaponProfile();
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        if (displaySize.x <= 0 || displaySize.y <= 0)
            return;

        ImVec2 screenCenter = GTA::GetCrosshairScreenPos();
        ImDrawList* draw = ImGui::GetForegroundDrawList();
        if (!draw) return;

        // 1. Círculo de FOV Próprio do Ragebot (Crimson Neon)
        if (profile.drawFov)
        {
            float fovRadius = GetFovRadius(profile.fov);
            draw->AddCircle(screenCenter, fovRadius, IM_COL32(255, 20, 55, 200), 64, 1.8f);
        }

        // 2. HUD Visual de Diagnóstico em tempo real (quando Debug Vector estiver ativo)
        if (profile.debugVector)
        {
            char line0[64], line1[64], line2[64], line3[64], line4[64], line5[64], line6[64], line7[64], line8[64], lineStatus[80];
            snprintf(line0, sizeof(line0), "[RAGE DEBUG]");
            snprintf(line1, sizeof(line1), "Enabled: %s", s_RageState.enabledPass ? "YES" : "NO");
            snprintf(line2, sizeof(line2), "Activation: %s", s_RageState.activationPass ? "YES" : "NO");
            if (s_RageState.targetId >= 0)
                snprintf(line3, sizeof(line3), "Target: #%d", s_RageState.targetId);
            else
                snprintf(line3, sizeof(line3), "Target: NONE");
            snprintf(line4, sizeof(line4), "Bone: %s", s_RageState.boneName);
            snprintf(line5, sizeof(line5), "FOV: %s (%.0f)", s_RageState.fovPass ? "PASS" : "FAIL", profile.fov);
            snprintf(line6, sizeof(line6), "Delta: %.1f / %.1f", s_RageState.deltaX, s_RageState.deltaY);
            snprintf(line7, sizeof(line7), "Aggressiveness: %.0f", profile.aggressiveness);
            snprintf(line8, sizeof(line8), "Output: %.1f / %.1f", s_RageState.stepX, s_RageState.stepY);
            snprintf(lineStatus, sizeof(lineStatus), "Status: %s", s_RageState.statusString);

            float startY = screenCenter.y + 35.0f;
            float startX = screenCenter.x + 20.0f;

            // Fundo escuro translúcido com borda neon para contraste perfeito
            draw->AddRectFilled(ImVec2(startX - 8, startY - 4), ImVec2(startX + 235, startY + 155), IM_COL32(10, 10, 15, 220), 4.0f);
            draw->AddRect(ImVec2(startX - 8, startY - 4), ImVec2(startX + 235, startY + 155), IM_COL32(255, 40, 70, 230), 4.0f, 0, 1.2f);

            draw->AddText(ImVec2(startX, startY), IM_COL32(255, 50, 80, 255), line0);
            draw->AddText(ImVec2(startX, startY + 15), s_RageState.enabledPass ? IM_COL32(50, 255, 120, 255) : IM_COL32(255, 70, 70, 255), line1);
            draw->AddText(ImVec2(startX, startY + 30), s_RageState.activationPass ? IM_COL32(50, 255, 120, 255) : IM_COL32(255, 200, 50, 255), line2);
            draw->AddText(ImVec2(startX, startY + 45), IM_COL32(255, 255, 255, 255), line3);
            draw->AddText(ImVec2(startX, startY + 60), IM_COL32(255, 200, 50, 255), line4);
            draw->AddText(ImVec2(startX, startY + 75), s_RageState.fovPass ? IM_COL32(50, 255, 120, 255) : IM_COL32(255, 180, 180, 255), line5);
            draw->AddText(ImVec2(startX, startY + 90), IM_COL32(220, 220, 220, 255), line6);
            draw->AddText(ImVec2(startX, startY + 105), IM_COL32(255, 120, 50, 255), line7);
            draw->AddText(ImVec2(startX, startY + 120), IM_COL32(50, 255, 120, 255), line8);

            ImU32 statusColor = s_RageState.isActive ? IM_COL32(50, 255, 120, 255) : IM_COL32(255, 200, 50, 255);
            draw->AddText(ImVec2(startX, startY + 135), statusColor, lineStatus);
        }

        // 3. Elementos visuais e vetor de depuração quando o alvo estiver ativo
        if (s_RageTarget.valid && s_RageState.isActive)
        {
            ImVec2 pos = s_RageTarget.screenPosition;
            float aggr = s_RageState.aggressiveness;
            float aggrRatio = aggr / 100.0f;
            const char* boneName = s_RageTarget.boneName ? s_RageTarget.boneName : "HEAD";

            // Target Indicator no osso visado
            if (profile.targetIndicator)
            {
                draw->AddCircle(pos, 9.0f, IM_COL32(255, 20, 50, 255), 24, 2.0f);
                draw->AddCircleFilled(pos, 3.5f, IM_COL32(255, 255, 255, 255));

                // Mira angular estilo Rage < + >
                draw->AddLine(ImVec2(pos.x - 15, pos.y), ImVec2(pos.x - 6, pos.y), IM_COL32(255, 30, 60, 255), 2.0f);
                draw->AddLine(ImVec2(pos.x + 6, pos.y), ImVec2(pos.x + 15, pos.y), IM_COL32(255, 30, 60, 255), 2.0f);
                draw->AddLine(ImVec2(pos.x, pos.y - 15), ImVec2(pos.x, pos.y - 6), IM_COL32(255, 30, 60, 255), 2.0f);
                draw->AddLine(ImVec2(pos.x, pos.y + 6), ImVec2(pos.x, pos.y + 15), IM_COL32(255, 30, 60, 255), 2.0f);

                char tag[96];
                snprintf(tag, sizeof(tag), "[RAGE #%d] %s (%.1fm)", s_RageTarget.playerId, boneName, s_RageTarget.distance3D);
                draw->AddText(ImVec2(pos.x + 18, pos.y - 10), IM_COL32(0, 0, 0, 255), tag);
                draw->AddText(ImVec2(pos.x + 17, pos.y - 11), IM_COL32(255, 40, 70, 255), tag);
            }

            // Vetor: CENTER -> TARGET HEAD
            if (profile.debugVector)
            {
                float vectorThickness = 1.2f + 2.8f * aggrRatio; // de 1.2f até 4.0f
                int vectorAlpha = 100 + static_cast<int>(155.0f * aggrRatio); // de 100 a 255
                draw->AddLine(screenCenter, pos, IM_COL32(255, 30, 60, vectorAlpha), vectorThickness);

                // Ponto e vetor de OUTPUT (demonstra a agressividade da puxada)
                ImVec2 outputPoint(screenCenter.x + s_RageState.stepX, screenCenter.y + s_RageState.stepY);
                draw->AddLine(screenCenter, outputPoint, IM_COL32(255, 230, 30, 255), 3.0f);
                draw->AddCircleFilled(outputPoint, 4.0f, IM_COL32(255, 230, 30, 255));
            }
        }
    }
}
