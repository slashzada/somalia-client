#include "SilentAim.h"
#include "../../Core/Logger.h"
#include "../../Core/RuntimeState.h"
#include "../../Engine/GTA/GTA.h"
#include "../../Engine/SAMP/SAMP.h"
#include "../Visuals/ESP.h"
#include <stdio.h>
#include <math.h>

namespace SilentAim
{
    static TargetInfo s_SilentTarget = {};
    static SilentAimState s_SilentState = {};
    static uint64_t s_LastLogTick = 0;
    static uint64_t s_LastShotTick = 0;
    static uint64_t s_LastSearchTick = 0;
    static int s_LastLoggedTargetId = -1;

    float GetFovRadius(float fovPercent)
    {
        // 100% de FOV equivale a 400 pixels na resolucao de tela (sincronizado com escala global)
        return fovPercent * 4.0f;
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

        return g_MenuState.silentAim.currentWeaponGroup;
    }

    SilentWeaponConfig& GetCurrentWeaponProfile()
    {
        if (g_MenuState.menuOpen)
        {
            int selected = g_MenuState.silentAim.currentWeaponGroup;
            if (selected < 0 || selected >= 4)
                selected = 0;
            return g_MenuState.silentAim.weapons[selected];
        }

        int activeGroup = GetActiveWeaponGroup();
        if (activeGroup < 0 || activeGroup >= 4)
            activeGroup = 0;

        return g_MenuState.silentAim.weapons[activeGroup];
    }

    const TargetInfo& GetCurrentTarget()
    {
        return s_SilentTarget;
    }

    const SilentAimState& GetState()
    {
        return s_SilentState;
    }

    void Reset()
    {
        s_SilentState = {};
        s_SilentState.targetId = -1;
        s_SilentTarget = {};
        s_SilentTarget.valid = false;
        s_LastLoggedTargetId = -1;
    }

    void ClearTarget()
    {
        s_SilentTarget = {};
        s_SilentTarget.valid = false;
        s_SilentState.isActive = false;
        s_SilentState.targetId = -1;
        s_LastLoggedTargetId = -1;
    }

    void RecordShotTick()
    {
        s_LastShotTick = GetTickCount64();
    }

    uint64_t GetLastShotTick()
    {
        return s_LastShotTick;
    }

    bool CheckActivationCondition(int activationMode)
    {
        if (g_MenuState.menuOpen)
            return false;

        bool isAiming = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        bool isShooting = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0) ||
                          ((GetTickCount64() - s_LastShotTick) < 150);

        switch (activationMode)
        {
        case 0: // Always
            return true;
        case 1: // While Aiming (RMB)
            return isAiming;
        case 2: // While Shooting (LMB)
            return isShooting;
        case 3: // Aim + Shoot
            return (isAiming && isShooting);
        default:
            return isShooting;
        }
    }

    int ResolveBoneIndex(int boneOption)
    {
        switch (boneOption)
        {
        case 0: return 8; // Head
        case 1: return 5; // Neck
        case 2: return 4; // Chest
        case 3: return 2; // Pelvis
        case 4: // Random Hitbox
        {
            int options[] = { 8, 5, 4, 2 };
            return options[rand() % 4];
        }
        default: return 8;
        }
    }

    const char* GetBoneName(int boneId)
    {
        switch (boneId)
        {
        case 8: return "HEAD";
        case 5: return "NECK";
        case 4: return "CHEST";
        case 2: return "PELVIS";
        default: return "BODY";
        }
    }

    static bool GetLocalEyePosition(float outEye[3])
    {
        __try
        {
            void* pLocalPed = *reinterpret_cast<void**>(0x00B7CD98);
            if (!pLocalPed) return false;

            float headPos[3] = { 0 };
            if (GTA::GetPedBonePosition(pLocalPed, 8, headPos))
            {
                outEye[0] = headPos[0];
                outEye[1] = headPos[1];
                outEye[2] = headPos[2];
                return true;
            }

            float pedPos[3] = { 0 };
            if (GTA::GetPedPosition(pLocalPed, pedPos))
            {
                outEye[0] = pedPos[0];
                outEye[1] = pedPos[1];
                outEye[2] = pedPos[2] + 0.72f;
                return true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return false;
    }

    void Update()
    {
        if (!RuntimeState::IsPlayerAlive())
        {
            ClearTarget();
            return;
        }

        // Monitora disparo do jogador local continuamente para telemetria de rede
        if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0)
        {
            RecordShotTick();
        }

        if (!g_MenuState.silentAim.enabled)
        {
            if (s_SilentTarget.valid)
            {
                Logger::Log("[SILENT] Desativado -> Limpando alvo (old=%d)", s_SilentTarget.playerId);
            }
            ClearTarget();
            return;
        }

        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        if (displaySize.x <= 0 || displaySize.y <= 0)
        {
            s_SilentTarget.valid = false;
            return;
        }

        // Garante integridade do hook do RakNet para interceptacao de BulletSync
        bool hookOk = SAMP::EnsureRakHook();
        static uint64_t s_lastHookDiagTick = 0;
        uint64_t nowTick = GetTickCount64();
        if (nowTick - s_lastHookDiagTick >= 4000)
        {
            Logger::Log("[SILENT][DIAG] EnsureRakHook() -> status=%s | hooked=%d",
                hookOk ? "OK" : "FALHA", SAMP::IsRakHooked() ? 1 : 0);
            s_lastHookDiagTick = nowTick;
        }

        int activeGroup = GetActiveWeaponGroup();
        if (activeGroup < 0 || activeGroup >= 4) activeGroup = 0;
        const auto& sw = g_MenuState.silentAim.weapons[activeGroup];

        if (!sw.enabled)
        {
            ClearTarget();
            return;
        }

        ImVec2 screenCenter = GTA::GetCrosshairScreenPos();
        float fovRadius = GetFovRadius(sw.fov);

        WeaponAimConfig selectorCfg = {};
        selectorCfg.enabled = true;
        selectorCfg.fov = sw.fov;
        selectorCfg.maxDistance = sw.maxDistance;
        selectorCfg.priority = sw.priority;
        selectorCfg.ignoreDead = sw.ignoreDead;
        selectorCfg.teamCheck = sw.teamCheck;
        selectorCfg.visibilityCheck = sw.visibilityCheck;

        // Resolve osso da arma atual (com amortecimento de randômico na tela a cada 300ms)
        if (sw.bone == 4)
        {
            static int s_lastRandomBoneTick = 0;
            static int s_cachedRandomBone = 0;
            if (s_lastRandomBoneTick == 0 || (nowTick - (uint64_t)s_lastRandomBoneTick) > 300)
            {
                int options[] = { 0, 1, 2, 3 };
                s_cachedRandomBone = options[rand() % 4];
                s_lastRandomBoneTick = (int)nowTick;
            }
            selectorCfg.bone = s_cachedRandomBone;
        }
        else
        {
            selectorCfg.bone = sw.bone;
        }

        bool needFullSearch = (nowTick - s_LastSearchTick >= 16);
        if (!s_SilentTarget.valid || !s_SilentTarget.ped || !GTA::IsPedAlive(s_SilentTarget.ped))
        {
            needFullSearch = true;
        }

        if (s_SilentTarget.valid && s_SilentTarget.ped && GTA::IsPedAlive(s_SilentTarget.ped))
        {
            float bonePos[3] = { 0 };
            if (GTA::GetPedBonePosition(s_SilentTarget.ped, s_SilentTarget.bone, bonePos) &&
                ESP::WorldToScreen(bonePos[0], bonePos[1], bonePos[2], s_SilentTarget.screenPosition))
            {
                s_SilentTarget.worldPosition[0] = bonePos[0];
                s_SilentTarget.worldPosition[1] = bonePos[1];
                s_SilentTarget.worldPosition[2] = bonePos[2];
                float dx = s_SilentTarget.screenPosition.x - screenCenter.x;
                float dy = s_SilentTarget.screenPosition.y - screenCenter.y;
                s_SilentTarget.distanceFromCrosshair = sqrtf(dx * dx + dy * dy);

                if (s_SilentTarget.distanceFromCrosshair > fovRadius)
                {
                    needFullSearch = true;
                }
            }
            else
            {
                needFullSearch = true;
            }
        }

        int candidates = 0;
        int insideFov = 0;

        if (needFullSearch)
        {
            s_LastSearchTick = nowTick;
            TargetInfo newTarget = TargetSelector::FindBestTarget(selectorCfg, screenCenter, fovRadius, candidates, insideFov, false);

            int newTargetId = newTarget.valid ? newTarget.playerId : -1;
            if (newTargetId != s_LastLoggedTargetId)
            {
                Logger::Log("[SILENT] Target changed: old=%d new=%d", s_LastLoggedTargetId, newTargetId);
                s_LastLoggedTargetId = newTargetId;
            }

            s_SilentTarget = newTarget;
        }

        // Atualiza estado de telemetria
        if (s_SilentTarget.valid)
        {
            s_SilentState.isActive = true;
            s_SilentState.targetId = s_SilentTarget.playerId;
            snprintf(s_SilentState.targetName, sizeof(s_SilentState.targetName), "%s", s_SilentTarget.name);
            snprintf(s_SilentState.boneName, sizeof(s_SilentState.boneName), "%s", s_SilentTarget.boneName);
            s_SilentState.targetWorldPos[0] = s_SilentTarget.worldPosition[0];
            s_SilentState.targetWorldPos[1] = s_SilentTarget.worldPosition[1];
            s_SilentState.targetWorldPos[2] = s_SilentTarget.worldPosition[2];
            s_SilentState.screenDist = s_SilentTarget.distanceFromCrosshair;
            s_SilentState.hitChance = sw.hitChance;
        }
        else
        {
            s_SilentState.isActive = false;
            s_SilentState.targetId = -1;
        }

        // Telemetria periódica throttled (~1s)
        if (nowTick - s_LastLogTick >= 1000)
        {
            if (s_SilentTarget.valid)
            {
                Logger::Log("[SILENT] candidates=%d inside_fov=%d selected_id=%d distance=%.1f screen_dist=%.1f bone=%s",
                    candidates, insideFov, s_SilentTarget.playerId, s_SilentTarget.distance3D, s_SilentTarget.distanceFromCrosshair, s_SilentTarget.boneName);
            }
            else
            {
                Logger::Log("[SILENT] candidates=%d inside_fov=%d selected_id=-1 distance=0.0 screen_dist=0.0 bone=NONE",
                    candidates, insideFov);
            }
            s_LastLogTick = nowTick;
        }
    }

    void Render()
    {
        // 1. Processa ciclo autônomo de alvos
        Update();

        if (!g_MenuState.silentAim.enabled) return;

        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        if (displaySize.x <= 0 || displaySize.y <= 0) return;

        int activeGroup = GetActiveWeaponGroup();
        if (activeGroup < 0 || activeGroup >= 4) activeGroup = 0;
        const auto& sw = g_MenuState.silentAim.weapons[activeGroup];

        if (!sw.enabled) return;

        ImDrawList* draw = ImGui::GetForegroundDrawList();
        if (!draw) return;

        ImVec2 screenCenter = GTA::GetCrosshairScreenPos();

        // 2. Renderização do Círculo de FOV dedicado do Silent Aim
        if (sw.drawFov)
        {
            float fovRadius = GetFovRadius(sw.fov);
            draw->AddCircle(screenCenter, fovRadius, IM_COL32(255, 130, 40, 180), 64, 1.5f);
        }

        // 3. Renderização do Indicador de Alvo [ O ] e Traçantes
        if (s_SilentTarget.valid && sw.targetIndicator)
        {
            ImVec2 pos = s_SilentTarget.screenPosition;

            // Marcador de anel centralizado no osso
            draw->AddCircle(pos, 8.0f, IM_COL32(255, 110, 30, 240), 20, 1.8f);
            draw->AddCircleFilled(pos, 2.5f, IM_COL32(255, 255, 255, 255));

            // Colchetes [ O ]
            draw->AddLine(ImVec2(pos.x - 14, pos.y), ImVec2(pos.x - 10, pos.y), IM_COL32(255, 110, 30, 230), 1.5f);
            draw->AddLine(ImVec2(pos.x + 10, pos.y), ImVec2(pos.x + 14, pos.y), IM_COL32(255, 110, 30, 230), 1.5f);

            // Rótulo textual com ID, osso e distância
            char tag[64];
            snprintf(tag, sizeof(tag), "[%d] %s (%.1fm)", s_SilentTarget.playerId, s_SilentTarget.boneName, s_SilentTarget.distance3D);
            draw->AddText(ImVec2(pos.x + 16, pos.y - 8), IM_COL32(0, 0, 0, 255), tag);
            draw->AddText(ImVec2(pos.x + 15, pos.y - 9), IM_COL32(255, 140, 50, 255), tag);

            // Traçante opcional
            if (sw.drawTracer)
            {
                draw->AddLine(screenCenter, pos, IM_COL32(255, 120, 30, 140), 1.0f);
            }
        }
    }

    SilentShotTarget FindTargetOnShot()
    {
        SilentShotTarget result = {};

        // 1. Jogador local vivo
        if (!RuntimeState::IsPlayerAlive())
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: Jogador local morto ou ped invalido");
            return result;
        }

        // 2. Silent master enable
        if (!g_MenuState.silentAim.enabled)
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: g_MenuState.silentAim.enabled == false");
            return result;
        }

        // 3. Grupo de arma e perfil
        int activeGroup = GetActiveWeaponGroup();
        if (activeGroup < 0 || activeGroup >= 4) activeGroup = 0;
        const auto& sw = g_MenuState.silentAim.weapons[activeGroup];
        if (!sw.enabled)
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: Perfil de arma desativado (grupo=%d, armaID=%u)",
                activeGroup, GTA::GetCurrentWeaponId());
            return result;
        }

        // 4. Modo de ativação
        if (!CheckActivationCondition(sw.activationMode))
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: Condicao de ativacao nao atendida (mode=%d)", sw.activationMode);
            return result;
        }

        // 5. Hit Chance
        if (sw.hitChance < 100)
        {
            int roll = rand() % 100;
            if (roll >= sw.hitChance)
            {
                Logger::Log("[SILENT][DIAG][FAIL] Motivo: HitChance falhou (roll=%d >= config=%d)", roll, sw.hitChance);
                return result;
            }
        }

        // 6. SAMP Carregado
        if (!SAMP::IsLoaded())
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: SAMP nao esta carregado");
            return result;
        }

        float localEye[3] = { 0 };
        GetLocalEyePosition(localEye);

        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        if (displaySize.x <= 0 || displaySize.y <= 0)
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: ImGui DisplaySize invalido (%.0f, %.0f)", displaySize.x, displaySize.y);
            return result;
        }

        ImVec2 screenCenter = GTA::GetCrosshairScreenPos();
        float fovRadius = GetFovRadius(sw.fov);

        // 7. Resolução do osso no instante exato do tiro
        int realBoneId = ResolveBoneIndex(sw.bone);
        const char* realBoneName = GetBoneName(realBoneId);

        // 8. Seleção FRESH de alvo no instante exato do disparo (não depende de frame anterior)
        WeaponAimConfig selectorCfg = {};
        selectorCfg.bone = sw.bone;
        selectorCfg.maxDistance = sw.maxDistance;
        selectorCfg.priority = sw.priority;
        selectorCfg.ignoreDead = sw.ignoreDead;
        selectorCfg.teamCheck = sw.teamCheck;
        selectorCfg.visibilityCheck = sw.visibilityCheck;
        selectorCfg.fov = sw.fov;

        int candidates = 0;
        int insideFov = 0;
        TargetInfo freshTarget = TargetSelector::FindBestTarget(selectorCfg, screenCenter, fovRadius, candidates, insideFov);

        if (!freshTarget.valid || freshTarget.playerId < 0)
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: TargetSelector nao encontrou alvo (candidatos=%d, dentroFOV=%d, fovRaio=%.1f px, maxDist=%.1fm)",
                candidates, insideFov, fovRadius, sw.maxDistance);
            return result;
        }

        // 9. Validação no pool do SA-MP
        SAMP::RemotePlayerData rpData;
        if (!SAMP::GetRemotePlayer(freshTarget.playerId, rpData) || !rpData.isValid)
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: RemotePlayer #%d invalido na pool SAMP", freshTarget.playerId);
            return result;
        }

        if (!rpData.isStreamed || !rpData.pGtaPed)
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: RemotePlayer #%d (%s) nao esta streamed ou ped nulo",
                freshTarget.playerId, rpData.name);
            return result;
        }

        if (!RuntimeState::IsValidPed(rpData.pGtaPed))
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: Ped do RemotePlayer #%d (%s) falhou em IsValidPed",
                freshTarget.playerId, rpData.name);
            return result;
        }

        if (sw.ignoreDead)
        {
            if (!GTA::IsPedAlive(rpData.pGtaPed) || rpData.health <= 0.0f)
            {
                Logger::Log("[SILENT][DIAG][FAIL] Motivo: RemotePlayer #%d (%s) esta morto (health=%.1f)",
                    freshTarget.playerId, rpData.name, rpData.health);
                return result;
            }
        }

        // 10. Obtenção do osso 3D do alvo
        float finalBonePos[3] = { 0 };
        bool gotBone = false;
        __try
        {
            gotBone = GTA::GetPedBonePosition(rpData.pGtaPed, realBoneId, finalBonePos);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { gotBone = false; }

        if (!gotBone)
        {
            finalBonePos[0] = rpData.position[0];
            finalBonePos[1] = rpData.position[1];
            float addZ = 0.2f;
            if (realBoneId == 8) addZ = 0.82f;
            else if (realBoneId == 5) addZ = 0.65f;
            else if (realBoneId == 4) addZ = 0.35f;
            finalBonePos[2] = rpData.position[2] + addZ;
        }

        result.valid = true;
        result.playerId = freshTarget.playerId;
        result.boneId = realBoneId;
        result.boneWorldPos[0] = finalBonePos[0];
        result.boneWorldPos[1] = finalBonePos[1];
        result.boneWorldPos[2] = finalBonePos[2];
        result.pedCenterPos[0] = rpData.position[0];
        result.pedCenterPos[1] = rpData.position[1];
        result.pedCenterPos[2] = rpData.position[2];
        strncpy(result.boneName, realBoneName, sizeof(result.boneName) - 1);
        strncpy(result.playerName, rpData.name[0] ? rpData.name : "Player", sizeof(result.playerName) - 1);

        Logger::Log("[SILENT][DIAG][SUCCESS] Alvo travado: ID #%d (%s) | Osso: %s | Pos3D: (%.1f, %.1f, %.1f)",
            result.playerId, result.playerName, result.boneName,
            result.boneWorldPos[0], result.boneWorldPos[1], result.boneWorldPos[2]);

        return result;
    }

    static float GetWeaponDamage(uint32_t weaponId)
    {
        switch (weaponId)
        {
        case 22: return 8.25f;   // 9mm (Colt 45)
        case 23: return 13.2f;   // Silenced 9mm
        case 24: return 46.2f;   // Desert Eagle
        case 25: return 30.0f;   // Shotgun
        case 26: return 30.0f;   // Sawn-off
        case 27: return 39.6f;   // SPAS-12 / Combat Shotgun
        case 28: return 6.6f;    // Micro UZI
        case 29: return 8.25f;   // MP5
        case 30: return 9.9f;    // AK-47
        case 31: return 9.9f;    // M4
        case 32: return 6.6f;    // Tec-9
        case 33: return 24.75f;  // Country Rifle
        case 34: return 41.58f;  // Sniper Rifle
        default: return 10.0f;
        }
    }

    static int GetBodyPartFromBone(int boneId)
    {
        switch (boneId)
        {
        case 8: return 9; // Head (SA-MP Bodypart 9)
        case 5: return 9; // Neck -> Head (Bodypart 9)
        case 4: return 3; // Chest (SA-MP Bodypart 3)
        case 2: return 4; // Crotch / Pelvis (SA-MP Bodypart 4)
        default: return 3;
        }
    }

    void Initialize()
    {
        GTA::SetWeaponFirePreHandler(&OnNativeWeaponFire);
        GTA::InstallWeaponHooks();
        Logger::Log("[SILENT] Modulo inicializado com hook de motor GTA SA (FireInstantHit).");
    }

    void Shutdown()
    {
        GTA::UninstallWeaponHooks();
        GTA::SetWeaponFirePreHandler(nullptr);
        ClearTarget();
        Logger::Log("[SILENT] Modulo descarregado com sucesso.");
    }

    bool OnNativeWeaponFire(void* pWeapon, void* pPed, void* pOrigin, void* pTarget,
                            float outSavedFront[3], float outSavedTarget[3], bool& outModifiedTarget)
    {
        outModifiedTarget = false;

        // 1. Apenas processa se o disparo partiu do Ped do jogador local
        void* pLocalPed = RuntimeState::GetLocalPed();
        if (!pLocalPed)
        {
            void** ppGta = reinterpret_cast<void**>(0x00B7CD98);
            if (ppGta) pLocalPed = *ppGta;
        }
        if (!pPed || pPed != pLocalPed)
        {
            return false;
        }

        // 2. Silent Master habilitado
        if (!g_MenuState.silentAim.enabled)
        {
            return false;
        }

        // 3. Perfil de arma habilitado
        int activeGroup = GetActiveWeaponGroup();
        if (activeGroup < 0 || activeGroup >= 4) activeGroup = 0;
        const auto& sw = g_MenuState.silentAim.weapons[activeGroup];
        if (!sw.enabled)
        {
            return false;
        }

        // 4. Modo de ativação satisfeito (Always, Aim, Shoot, Aim+Shoot)
        if (!CheckActivationCondition(sw.activationMode))
        {
            return false;
        }

        // 5. HitChance
        if (sw.hitChance < 100)
        {
            int roll = rand() % 100;
            if (roll >= sw.hitChance)
            {
                return false;
            }
        }

        // 6. Localiza o melhor alvo no instante exato do disparo (Fresh Target)
        SilentShotTarget shot = FindTargetOnShot();
        if (!shot.valid)
        {
            return false;
        }

        // 7. Redirecionamento nativo do vetor frontal da câmera do GTA SA
        // IMPORTANTE: A MIRA DO JOGADOR NÃO É PUXADA. A TELA PERMANECE TOTALMENTE ESTÁTICA!
        // Apenas o vetor interno de cálculo do raycast e do traçante é temporariamente alinhado
        // ao osso do alvo durante a chamada do disparo, sendo restaurado imediatamente ao retornar.
        float camSource[3] = { 0.0f, 0.0f, 0.0f };
        if (!GTA::GetCameraFront(outSavedFront) || !GTA::GetCameraSource(camSource))
        {
            return false;
        }

        float dir[3] = {
            shot.boneWorldPos[0] - camSource[0],
            shot.boneWorldPos[1] - camSource[1],
            shot.boneWorldPos[2] - camSource[2]
        };
        float len = sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
        if (len <= 0.001f)
        {
            return false;
        }

        float newFront[3] = { dir[0] / len, dir[1] / len, dir[2] / len };
        GTA::SetCameraFront(newFront);

        // 8. Se pTarget foi repassado pelo motor (em algumas armas), redireciona-o também
        float* pTargetVec = reinterpret_cast<float*>(pTarget);
        if (pTargetVec && !IsBadWritePtr(pTargetVec, sizeof(float) * 3))
        {
            outSavedTarget[0] = pTargetVec[0];
            outSavedTarget[1] = pTargetVec[1];
            outSavedTarget[2] = pTargetVec[2];
            pTargetVec[0] = shot.boneWorldPos[0];
            pTargetVec[1] = shot.boneWorldPos[1];
            pTargetVec[2] = shot.boneWorldPos[2];
            outModifiedTarget = true;
        }

        uint32_t weaponId = GTA::GetCurrentWeaponId();
        Logger::Log("[SILENT][GTA_HOOK] Disparo nativo do motor GTA SA redirecionado para Alvo #%d (%s) Osso=%s Arma=%u Dist=%.1fm (MIRA INTACTA)",
            shot.playerId, shot.playerName, shot.boneName, weaponId, len);
        return true;
    }

    void MutateBulletSyncPacket(unsigned char* data, int length)
    {
        if (!data || length < 40)
        {
            Logger::Log("[SILENT][DIAG][MUTATE] MutateBulletSyncPacket abortado: data=%p length=%d (<40)", data, length);
            return;
        }
        if (data[0] != 206)
        {
            Logger::Log("[SILENT][DIAG][MUTATE] MutateBulletSyncPacket abortado: data[0]=%u (!=206)", data[0]);
            return;
        }

        Logger::Log("[SILENT][DIAG][MUTATE] MutateBulletSyncPacket iniciado (packetId=206, length=%d)", length);

        SilentShotTarget shot = FindTargetOnShot();
        if (!shot.valid)
        {
            Logger::Log("[SILENT][DIAG][MUTATE] FindTargetOnShot retornou INVALIDO -> Disparo original mantido.");

            // Sky Bullet Sync para disparos legítimos (mira manual sem Silent Aim):
            if (g_MenuState.silentAim.skyBulletSync && data[1] == 1) // BULLET_HIT_TYPE_PLAYER
            {
                uint8_t weaponId = 0;
                if (length >= 41) weaponId = data[40];
                if (weaponId == 0) weaponId = static_cast<uint8_t>(GTA::GetCurrentWeaponId());

                float centerZ = *reinterpret_cast<float*>(data + 36);
                bool isSniper = (weaponId == 34);
                bool isHeadshot = (centerZ >= 0.35f);

                float targetX = *reinterpret_cast<float*>(data + 16);
                float targetY = *reinterpret_cast<float*>(data + 20);
                float targetZ = *reinterpret_cast<float*>(data + 24);

                float localPos[3] = { 0.0f, 0.0f, 0.0f };
                SAMP::GetLocalPlayerPosition(localPos);
                float dx = targetX - localPos[0];
                float dy = targetY - localPos[1];
                float dz = targetZ - localPos[2];
                float distFromMe = sqrtf(dx * dx + dy * dy + dz * dz);

                if (isSniper && isHeadshot && distFromMe >= 65.0f)
                {
                    float skyH = g_MenuState.silentAim.skyHeight;
                    if (skyH < 50.0f) skyH = 75.0f;

                    *reinterpret_cast<float*>(data + 4)  = targetX;
                    *reinterpret_cast<float*>(data + 8)  = targetY;
                    *reinterpret_cast<float*>(data + 12) = targetZ + skyH;

                    uint16_t targetId = *reinterpret_cast<uint16_t*>(data + 2);
                    Logger::Log("[SKY_BULLET][LEGIT] Sky Bullet Sync ativado em tiro manual! Alvo=#%d fOrigin=(%.1f, %.1f, +%.1fm ceu) dist=%.1fm",
                        targetId, targetX, targetY, targetZ + skyH, distFromMe);

                    SAMP::SendGiveDamage(targetId, 41.6f, 34, 9);
                }
            }
            return;
        }

        // 1. Tipo de acerto: Jogador
        data[1] = 1; // BULLET_HIT_TYPE_PLAYER
        *reinterpret_cast<uint16_t*>(data + 2) = static_cast<uint16_t>(shot.playerId);

        // 2. fOrigin (data + 4 .. 12):
        // PRESERVA o fOrigin original calculado pelo SA-MP (cano legítimo da arma)!
        // Se estiver zerado por algum motivo raro, usa a posição dos olhos
        float origX = *reinterpret_cast<float*>(data + 4);
        float origY = *reinterpret_cast<float*>(data + 8);
        float origZ = *reinterpret_cast<float*>(data + 12);
        if (fabsf(origX) < 0.01f && fabsf(origY) < 0.01f && fabsf(origZ) < 0.01f)
        {
            float eyePos[3] = { 0 };
            GetLocalEyePosition(eyePos);
            *reinterpret_cast<float*>(data + 4)  = eyePos[0];
            *reinterpret_cast<float*>(data + 8)  = eyePos[1];
            *reinterpret_cast<float*>(data + 12) = eyePos[2];
            origX = eyePos[0];
            origY = eyePos[1];
            origZ = eyePos[2];
        }

        // 3. fTarget (data + 16 .. 24): Posição 3D absoluta do osso no mundo
        *reinterpret_cast<float*>(data + 16) = shot.boneWorldPos[0];
        *reinterpret_cast<float*>(data + 20) = shot.boneWorldPos[1];
        *reinterpret_cast<float*>(data + 24) = shot.boneWorldPos[2];

        // 4. fCenter (data + 28 .. 36): Offset LOCAL autêntico relativo à hitbox do Ped
        // No protocolo SA-MP stBulletData, fCenter é estritamente no espaço LOCAL do Ped:
        // Cabeça (8) = {0.0, 0.0, 0.68}
        // Pescoço (5) = {0.0, 0.0, 0.55}
        // Peito (4) = {0.0, 0.0, 0.35}
        // Pélvis (2) = {0.0, 0.0, 0.0}
        float localZ = 0.68f;
        if (shot.boneId == 5) localZ = 0.55f;
        else if (shot.boneId == 4) localZ = 0.35f;
        else if (shot.boneId == 2) localZ = 0.0f;

        *reinterpret_cast<float*>(data + 28) = 0.0f;
        *reinterpret_cast<float*>(data + 32) = 0.0f;
        *reinterpret_cast<float*>(data + 36) = localZ;

        // 5. Arma utilizada (data + 40 se length >= 41)
        uint8_t weaponId = 0;
        if (length >= 41)
        {
            weaponId = data[40];
        }
        if (weaponId == 0)
        {
            weaponId = static_cast<uint8_t>(GTA::GetCurrentWeaponId());
        }

        // Checagem de Sky Bullet Sync para Silent Aim
        if (g_MenuState.silentAim.skyBulletSync)
        {
            float localPos[3] = { 0.0f, 0.0f, 0.0f };
            SAMP::GetLocalPlayerPosition(localPos);
            float dx = shot.boneWorldPos[0] - localPos[0];
            float dy = shot.boneWorldPos[1] - localPos[1];
            float dz = shot.boneWorldPos[2] - localPos[2];
            float distFromMe = sqrtf(dx * dx + dy * dy + dz * dz);

            bool isSniper = (weaponId == 34);
            bool isHeadshot = (shot.boneId == 0 || shot.boneId == 8 || localZ >= 0.35f);

            if (isSniper && isHeadshot && distFromMe >= 65.0f)
            {
                float skyH = g_MenuState.silentAim.skyHeight;
                if (skyH < 50.0f) skyH = 75.0f;

                *reinterpret_cast<float*>(data + 4)  = shot.boneWorldPos[0];
                *reinterpret_cast<float*>(data + 8)  = shot.boneWorldPos[1];
                *reinterpret_cast<float*>(data + 12) = shot.boneWorldPos[2] + skyH;

                Logger::Log("[SKY_BULLET][SILENT] Sky Bullet Sync ativado no Silent Aim! Alvo=#%d (%s) fOrigin=(%.1f, %.1f, +%.1fm ceu) dist=%.1fm",
                    shot.playerId, shot.playerName, shot.boneWorldPos[0], shot.boneWorldPos[1], shot.boneWorldPos[2] + skyH, distFromMe);
            }
        }

        // 6. Transmissão imediata de RPC 115 (SendGiveDamage) para garantir o registro do dano no servidor
        float damage = GetWeaponDamage(weaponId);
        int bodyPart = GetBodyPartFromBone(shot.boneId);
        SAMP::SendGiveDamage(shot.playerId, damage, weaponId, bodyPart);

        float dx = shot.boneWorldPos[0] - origX;
        float dy = shot.boneWorldPos[1] - origY;
        float dz = shot.boneWorldPos[2] - origZ;
        float dist3D = sqrtf(dx * dx + dy * dy + dz * dz);

        Logger::Log("[SILENT][DIAG][REDIRECT] SHOT REDIRECTED -> target=%d (%s) bone=%s dist=%.1fm weapon=%d dmg=%.1f bodypart=%d targetBone=(%.1f,%.1f,%.1f)",
            shot.playerId, shot.playerName, shot.boneName, dist3D,
            weaponId, damage, bodyPart,
            shot.boneWorldPos[0], shot.boneWorldPos[1], shot.boneWorldPos[2]);

        // Atualiza telemetria de diagnóstico
        s_SilentState.isActive = true;
        s_SilentState.targetId = shot.playerId;
        snprintf(s_SilentState.targetName, sizeof(s_SilentState.targetName), "%s", shot.playerName);
        snprintf(s_SilentState.boneName, sizeof(s_SilentState.boneName), "%s", shot.boneName);
        s_SilentState.targetWorldPos[0] = shot.boneWorldPos[0];
        s_SilentState.targetWorldPos[1] = shot.boneWorldPos[1];
        s_SilentState.targetWorldPos[2] = shot.boneWorldPos[2];
        s_SilentState.predictedImpact[0] = shot.boneWorldPos[0];
        s_SilentState.predictedImpact[1] = shot.boneWorldPos[1];
        s_SilentState.predictedImpact[2] = shot.boneWorldPos[2];
        s_SilentState.hitChancePassed = true;
        s_SilentState.lastShotTick = GetTickCount64();
    }
}
