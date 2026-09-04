#include "ESP.h"
#include "../../Config/Config.h"
#include "../../Engine/SAMP/SAMP.h"
#include "../../Engine/GTA/GTA.h"
#include "../../Core/Logger.h"
#include "../../Core/RuntimeState.h"
#include "../../Render/ImGui/imgui_internal.h"
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <vector>

namespace ESP
{
    // Enderecos estaticos do RenderWare / GTA SA 1.0 US para projecao de matriz 3D->2D
    static constexpr uintptr_t ADDR_RW_VIEWPROJ_MATRIX = 0x00B6FA2C;
    static constexpr uintptr_t ADDR_RW_VIEWPORT_WIDTH  = 0x00C17044;
    static constexpr uintptr_t ADDR_RW_VIEWPORT_HEIGHT = 0x00C17048;

    static uint64_t s_LastLogTick = 0;
    static bool s_DrawListTestLogged = false;

    // Estado do Hitmarker
    static ULONGLONG s_HitmarkerTick = 0;

    // Estado do Damage Informer
    struct DamageInformerEntry
    {
        float x, y, z;
        float damage;
        ULONGLONG spawnTick;
    };
    static std::vector<DamageInformerEntry> s_DamageEntries;

    // Rastreamento de vida/colete dos jogadores para detecao de acertos
    static float s_PrevHealthArmor[1004] = { 0.0f };

    void TriggerHitmarker()
    {
        s_HitmarkerTick = GetTickCount64();
    }

    void AddDamageInformer(float worldX, float worldY, float worldZ, float damage)
    {
        DamageInformerEntry e;
        e.x = worldX;
        e.y = worldY;
        e.z = worldZ;
        e.damage = damage;
        e.spawnTick = GetTickCount64();
        s_DamageEntries.push_back(e);

        if (s_DamageEntries.size() > 40)
        {
            s_DamageEntries.erase(s_DamageEntries.begin());
        }
    }

    bool WorldToScreen(float worldX, float worldY, float worldZ, ImVec2& outScreen)
    {
        __try
        {
            float* m = reinterpret_cast<float*>(ADDR_RW_VIEWPROJ_MATRIX);
            if (!m) return false;

            DWORD* pWidth  = reinterpret_cast<DWORD*>(ADDR_RW_VIEWPORT_WIDTH);
            DWORD* pHeight = reinterpret_cast<DWORD*>(ADDR_RW_VIEWPORT_HEIGHT);
            if (!pWidth || !pHeight || *pWidth == 0 || *pHeight == 0) return false;

            float screenX = (worldZ * m[8])  + (worldY * m[4]) + (worldX * m[0]) + m[12];
            float screenY = (worldZ * m[9])  + (worldY * m[5]) + (worldX * m[1]) + m[13];
            float screenZ = (worldZ * m[10]) + (worldY * m[6]) + (worldX * m[2]) + m[14];

            if (screenZ < 0.1f)
                return false;

            float fRecip = 1.0f / screenZ;
            screenX *= (fRecip * static_cast<float>(*pWidth));
            screenY *= (fRecip * static_cast<float>(*pHeight));

            if (isnan(screenX) || isinf(screenX) || isnan(screenY) || isinf(screenY))
                return false;

            outScreen = ImVec2(screenX, screenY);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    static void DrawCornerBox(ImDrawList* draw, ImVec2 min, ImVec2 max, ImU32 color, float thickness)
    {
        float w = max.x - min.x;
        float h = max.y - min.y;
        float lineW = w * 0.25f;
        float lineH = h * 0.25f;

        // Top Left
        draw->AddLine(min, ImVec2(min.x + lineW, min.y), color, thickness);
        draw->AddLine(min, ImVec2(min.x, min.y + lineH), color, thickness);

        // Top Right
        draw->AddLine(ImVec2(max.x, min.y), ImVec2(max.x - lineW, min.y), color, thickness);
        draw->AddLine(ImVec2(max.x, min.y), ImVec2(max.x, min.y + lineH), color, thickness);

        // Bottom Left
        draw->AddLine(ImVec2(min.x, max.y), ImVec2(min.x + lineW, max.y), color, thickness);
        draw->AddLine(ImVec2(min.x, max.y), ImVec2(min.x, max.y - lineH), color, thickness);

        // Bottom Right
        draw->AddLine(max, ImVec2(max.x - lineW, max.y), color, thickness);
        draw->AddLine(max, ImVec2(max.x, max.y - lineH), color, thickness);
    }

    static void DrawHealthBar(ImDrawList* draw, ImVec2 min, ImVec2 max, float health, float maxHealth)
    {
        if (maxHealth <= 0.0f) maxHealth = 100.0f;
        float ratio = ImClamp(health / maxHealth, 0.0f, 1.0f);

        float barWidth = 3.0f;
        float barSpacing = 2.0f;
        float h = max.y - min.y;

        ImVec2 barMin(min.x - barSpacing - barWidth, min.y);
        ImVec2 barMax(min.x - barSpacing, max.y);

        draw->AddRectFilled(ImVec2(barMin.x - 1, barMin.y - 1), ImVec2(barMax.x + 1, barMax.y + 1), IM_COL32(0, 0, 0, 200));

        float fillHeight = h * ratio;
        ImVec2 fillMin(barMin.x, max.y - fillHeight);

        ImU32 healthColor = IM_COL32(int((1.0f - ratio) * 255), int(ratio * 255), 40, 255);
        draw->AddRectFilled(fillMin, barMax, healthColor);
    }

    static void DrawArmorBar(ImDrawList* draw, ImVec2 min, ImVec2 max, float armor, float maxArmor)
    {
        if (armor <= 0.0f) return;
        if (maxArmor <= 0.0f) maxArmor = 100.0f;
        float ratio = ImClamp(armor / maxArmor, 0.0f, 1.0f);

        float barWidth = 3.0f;
        float barSpacing = 6.0f;
        float h = max.y - min.y;

        ImVec2 barMin(min.x - barSpacing - barWidth, min.y);
        ImVec2 barMax(min.x - barSpacing, max.y);

        draw->AddRectFilled(ImVec2(barMin.x - 1, barMin.y - 1), ImVec2(barMax.x + 1, barMax.y + 1), IM_COL32(0, 0, 0, 200));

        float fillHeight = h * ratio;
        ImVec2 fillMin(barMin.x, max.y - fillHeight);
        draw->AddRectFilled(fillMin, barMax, IM_COL32(65, 140, 240, 255));
    }

    static void DrawBoneLine(ImDrawList* draw, void* pPed, int b1, int b2, ImU32 color)
    {
        float p1[3], p2[3];
        if (!GTA::GetPedBonePosition(pPed, b1, p1)) return;
        if (!GTA::GetPedBonePosition(pPed, b2, p2)) return;

        ImVec2 s1, s2;
        if (!WorldToScreen(p1[0], p1[1], p1[2], s1)) return;
        if (!WorldToScreen(p2[0], p2[1], p2[2], s2)) return;

        draw->AddLine(s1, s2, color, 1.2f);
    }

    static void DrawSkeleton(ImDrawList* draw, void* pPed, ImU32 color)
    {
        DrawBoneLine(draw, pPed, 8, 5, color);  // Head -> Neck
        DrawBoneLine(draw, pPed, 5, 4, color);  // Neck -> Upper Torso
        DrawBoneLine(draw, pPed, 4, 3, color);  // Upper Torso -> Spine
        DrawBoneLine(draw, pPed, 3, 2, color);  // Spine -> Pelvis

        DrawBoneLine(draw, pPed, 4, 32, color);  // Upper Torso -> Left Shoulder
        DrawBoneLine(draw, pPed, 32, 33, color); // Left Shoulder -> Left Elbow
        DrawBoneLine(draw, pPed, 33, 35, color); // Left Elbow -> Left Hand

        DrawBoneLine(draw, pPed, 4, 22, color);  // Upper Torso -> Right Shoulder
        DrawBoneLine(draw, pPed, 22, 23, color); // Right Shoulder -> Right Elbow
        DrawBoneLine(draw, pPed, 23, 25, color); // Right Elbow -> Right Hand

        DrawBoneLine(draw, pPed, 2, 41, color);  // Pelvis -> Left Hip
        DrawBoneLine(draw, pPed, 41, 42, color); // Left Hip -> Left Knee
        DrawBoneLine(draw, pPed, 42, 44, color); // Left Knee -> Left Foot

        DrawBoneLine(draw, pPed, 2, 51, color);  // Pelvis -> Right Hip
        DrawBoneLine(draw, pPed, 51, 52, color); // Right Hip -> Right Knee
        DrawBoneLine(draw, pPed, 52, 54, color); // Right Knee -> Right Foot
    }

    static void RenderHitmarker(ImDrawList* draw, ImVec2 screenCenter, ULONGLONG currentTick)
    {
        if (s_HitmarkerTick == 0) return;
        ULONGLONG diff = currentTick - s_HitmarkerTick;
        if (diff > 400) return;

        float progress = static_cast<float>(diff) / 400.0f;
        int alpha = static_cast<int>(255.0f * (1.0f - progress));
        if (alpha <= 0) return;

        ImU32 col = IM_COL32(255, 55, 65, alpha);
        float gap = 3.5f;
        float len = 8.5f;

        draw->AddLine(ImVec2(screenCenter.x - gap - len, screenCenter.y - gap - len), ImVec2(screenCenter.x - gap, screenCenter.y - gap), col, 1.8f);
        draw->AddLine(ImVec2(screenCenter.x + gap + len, screenCenter.y - gap - len), ImVec2(screenCenter.x + gap, screenCenter.y - gap), col, 1.8f);
        draw->AddLine(ImVec2(screenCenter.x - gap - len, screenCenter.y + gap + len), ImVec2(screenCenter.x - gap, screenCenter.y + gap), col, 1.8f);
        draw->AddLine(ImVec2(screenCenter.x + gap + len, screenCenter.y + gap + len), ImVec2(screenCenter.x + gap, screenCenter.y + gap), col, 1.8f);
    }

    static void RenderDamageInformer(ImDrawList* draw, ULONGLONG currentTick)
    {
        for (auto it = s_DamageEntries.begin(); it != s_DamageEntries.end(); )
        {
            ULONGLONG age = currentTick - it->spawnTick;
            if (age > 1200)
            {
                it = s_DamageEntries.erase(it);
                continue;
            }

            float progress = static_cast<float>(age) / 1200.0f;
            int alpha = static_cast<int>(255.0f * (1.0f - progress));
            float rise = progress * 0.75f;

            ImVec2 screenPos;
            if (WorldToScreen(it->x, it->y, it->z + rise + 0.35f, screenPos))
            {
                char dmgStr[32];
                snprintf(dmgStr, sizeof(dmgStr), "-%.0f HP", it->damage);

                ImVec2 sz = ImGui::CalcTextSize(dmgStr);
                ImVec2 pos(screenPos.x - sz.x * 0.5f, screenPos.y);

                draw->AddText(ImVec2(pos.x + 1, pos.y + 1), IM_COL32(0, 0, 0, alpha), dmgStr);
                draw->AddText(pos, IM_COL32(255, 65, 65, alpha), dmgStr);
            }

            ++it;
        }
    }

    static void RenderVehiclesESP(ImDrawList* draw, const float localPos[3])
    {
        if (!g_MenuState.visuals.vehicleESP) return;

        __try
        {
            uintptr_t pPool = *reinterpret_cast<uintptr_t*>(0x00B74494);
            if (!pPool) return;

            void** ppVehicles = *reinterpret_cast<void***>(pPool);
            uint8_t* pFlags = *reinterpret_cast<uint8_t**>(pPool + 0x4);
            int maxVehicles = *reinterpret_cast<int*>(pPool + 0x8);

            if (!ppVehicles || !pFlags || maxVehicles <= 0 || maxVehicles > 2500) return;

            void* pLocalVeh = nullptr;
            void* pLocalPed = *reinterpret_cast<void**>(0x00B7CD98);
            if (pLocalPed)
            {
                pLocalVeh = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(pLocalPed) + 0x58C);
            }

            for (int v = 0; v < maxVehicles; v++)
            {
                if (pFlags[v] & 0x80) continue;
                void* pVeh = ppVehicles[v];
                if (!pVeh || pVeh == pLocalVeh) continue;

                uintptr_t vehAddr = reinterpret_cast<uintptr_t>(pVeh);
                uintptr_t pMatrix = *reinterpret_cast<uintptr_t*>(vehAddr + 0x14);

                float vx = *(float*)(pMatrix ? pMatrix + 0x30 : vehAddr + 0x4);
                float vy = *(float*)(pMatrix ? pMatrix + 0x34 : vehAddr + 0x8);
                float vz = *(float*)(pMatrix ? pMatrix + 0x38 : vehAddr + 0xC);

                float dx = vx - localPos[0];
                float dy = vy - localPos[1];
                float dz = vz - localPos[2];
                float dist = sqrtf(dx * dx + dy * dy + dz * dz);

                if (dist > static_cast<float>(g_MenuState.visuals.maxDistance)) continue;

                ImVec2 sPos;
                if (WorldToScreen(vx, vy, vz, sPos))
                {
                    float hp = *reinterpret_cast<float*>(vehAddr + 0x4C0);
                    uint16_t modelId = *reinterpret_cast<uint16_t*>(vehAddr + 0x22);

                    char vehBuf[48];
                    snprintf(vehBuf, sizeof(vehBuf), "Vehicle [%d] (%.0f HP) - %.0fm", modelId, hp, dist);
                    ImVec2 txtSz = ImGui::CalcTextSize(vehBuf);

                    draw->AddRectFilled(ImVec2(sPos.x - txtSz.x * 0.5f - 3, sPos.y - 2), ImVec2(sPos.x + txtSz.x * 0.5f + 3, sPos.y + txtSz.y + 2), IM_COL32(15, 15, 20, 200), 2.0f);
                    draw->AddText(ImVec2(sPos.x - txtSz.x * 0.5f, sPos.y), IM_COL32(255, 215, 60, 240), vehBuf);
                    draw->AddCircle(sPos, 3.5f, IM_COL32(255, 215, 60, 255), 10, 1.2f);
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    static void RenderPickupsESP(ImDrawList* draw, const float localPos[3])
    {
        if (!g_MenuState.visuals.pickupESP) return;

        __try
        {
            uintptr_t pickupArray = 0x009788C0;
            for (int p = 0; p < 620; p++)
            {
                uintptr_t pPick = pickupArray + p * 0x20;
                uint8_t state = *reinterpret_cast<uint8_t*>(pPick + 0x14);
                if (state == 0) continue;

                float px = *reinterpret_cast<float*>(pPick + 0x0);
                float py = *reinterpret_cast<float*>(pPick + 0x4);
                float pz = *reinterpret_cast<float*>(pPick + 0x8);
                if (px == 0.0f && py == 0.0f && pz == 0.0f) continue;

                float dx = px - localPos[0];
                float dy = py - localPos[1];
                float dz = pz - localPos[2];
                float dist = sqrtf(dx * dx + dy * dy + dz * dz);

                if (dist > static_cast<float>(g_MenuState.visuals.maxDistance)) continue;

                ImVec2 sPos;
                if (WorldToScreen(px, py, pz, sPos))
                {
                    uint16_t model = *reinterpret_cast<uint16_t*>(pPick + 0x10);
                    char pickBuf[40];
                    snprintf(pickBuf, sizeof(pickBuf), "Item [%d] - %.0fm", model, dist);
                    ImVec2 txtSz = ImGui::CalcTextSize(pickBuf);

                    draw->AddRectFilled(ImVec2(sPos.x - txtSz.x * 0.5f - 2, sPos.y - 2), ImVec2(sPos.x + txtSz.x * 0.5f + 2, sPos.y + txtSz.y + 2), IM_COL32(10, 10, 15, 190), 2.0f);
                    draw->AddText(ImVec2(sPos.x - txtSz.x * 0.5f, sPos.y), IM_COL32(90, 255, 140, 240), pickBuf);
                    draw->AddCircle(sPos, 3.0f, IM_COL32(90, 255, 140, 255), 8, 1.2f);
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    static void RenderObjectsESP(ImDrawList* draw, const float localPos[3])
    {
        if (!g_MenuState.visuals.objectESP) return;

        __try
        {
            uintptr_t pObjPool = *reinterpret_cast<uintptr_t*>(0x00B7449C);
            if (!pObjPool) return;

            void** ppObjs = *reinterpret_cast<void***>(pObjPool);
            uint8_t* pFlags = *reinterpret_cast<uint8_t**>(pObjPool + 0x4);
            int maxObjs = *reinterpret_cast<int*>(pObjPool + 0x8);

            if (!ppObjs || !pFlags || maxObjs <= 0 || maxObjs > 2000) return;

            for (int o = 0; o < maxObjs; o++)
            {
                if (pFlags[o] & 0x80) continue;
                void* pObj = ppObjs[o];
                if (!pObj) continue;

                uintptr_t objAddr = reinterpret_cast<uintptr_t>(pObj);
                uintptr_t pMatrix = *reinterpret_cast<uintptr_t*>(objAddr + 0x14);

                float ox = *(float*)(pMatrix ? pMatrix + 0x30 : objAddr + 0x4);
                float oy = *(float*)(pMatrix ? pMatrix + 0x34 : objAddr + 0x8);
                float oz = *(float*)(pMatrix ? pMatrix + 0x38 : objAddr + 0xC);

                float dx = ox - localPos[0];
                float dy = oy - localPos[1];
                float dz = oz - localPos[2];
                float dist = sqrtf(dx * dx + dy * dy + dz * dz);

                if (dist > static_cast<float>(g_MenuState.visuals.maxDistance) || dist > 85.0f) continue;

                ImVec2 sPos;
                if (WorldToScreen(ox, oy, oz, sPos))
                {
                    uint16_t model = *reinterpret_cast<uint16_t*>(objAddr + 0x22);
                    char objBuf[32];
                    snprintf(objBuf, sizeof(objBuf), "Object [%d] %.0fm", model, dist);
                    draw->AddText(sPos, IM_COL32(180, 180, 255, 200), objBuf);
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void Render()
    {
        ImDrawList* draw = ImGui::GetForegroundDrawList();
        if (!draw) return;

        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        if (displaySize.x <= 0 || displaySize.y <= 0) return;

        ImVec2 screenCenter = GTA::GetCrosshairScreenPos();
        ULONGLONG currentTick = GetTickCount64();

        // 1. Render FOV Circle
        if (g_MenuState.visuals.drawFOVCircle)
        {
            float radius = static_cast<float>(g_MenuState.visuals.fovCircleRadius) * 4.0f;
            draw->AddCircle(screenCenter, radius, IM_COL32(137, 207, 240, 180), 64, 1.2f);
        }

        // 1.1 Custom Screen Crosshair
        if (g_MenuState.visuals.customCrosshair)
        {
            float crossLen = 6.0f;
            float crossGap = 3.0f;
            draw->AddCircleFilled(screenCenter, 1.5f, IM_COL32(255, 60, 90, 255));
            draw->AddLine(ImVec2(screenCenter.x - crossGap - crossLen, screenCenter.y), ImVec2(screenCenter.x - crossGap, screenCenter.y), IM_COL32(255, 60, 90, 240), 1.5f);
            draw->AddLine(ImVec2(screenCenter.x + crossGap, screenCenter.y), ImVec2(screenCenter.x + crossGap + crossLen, screenCenter.y), IM_COL32(255, 60, 90, 240), 1.5f);
            draw->AddLine(ImVec2(screenCenter.x - crossGap - crossLen, screenCenter.y), ImVec2(screenCenter.x - crossGap, screenCenter.y), IM_COL32(255, 60, 90, 240), 1.5f);
            draw->AddLine(ImVec2(screenCenter.x + crossGap, screenCenter.y), ImVec2(screenCenter.x + crossGap + crossLen, screenCenter.y), IM_COL32(255, 60, 90, 240), 1.5f);
        }

        // 1.2 Hitmarker on Damage
        if (g_MenuState.visuals.hitmarker)
        {
            RenderHitmarker(draw, screenCenter, currentTick);
        }

        // 1.3 Damage Informer Flutuante
        if (g_MenuState.visuals.damageInformer)
        {
            RenderDamageInformer(draw, currentTick);
        }

        // 1.4 Indicador Invertebred em Execucao
        if (g_MenuState.antiAim.invertebred)
        {
            draw->AddRectFilled(ImVec2(20, 20), ImVec2(195, 46), IM_COL32(20, 20, 20, 210), 4.0f);
            draw->AddRect(ImVec2(20, 20), ImVec2(195, 46), IM_COL32(137, 207, 240, 220), 4.0f, 0, 1.2f);
            draw->AddText(ImVec2(28, 26), IM_COL32(137, 207, 240, 255), "INVERTEBRED: ATIVO");
        }

        // 2. Se o Master ESP estiver desligado, encerra o ciclo
        if (!g_MenuState.visuals.enableESP)
        {
            s_DrawListTestLogged = false;
            return;
        }

        // 3. Teste Obrigatorio de DrawList
        draw->AddRectFilled(ImVec2(screenCenter.x - 90, 20), ImVec2(screenCenter.x + 90, 48), IM_COL32(20, 20, 20, 220), 4.0f);
        draw->AddRect(ImVec2(screenCenter.x - 90, 20), ImVec2(screenCenter.x + 90, 48), IM_COL32(80, 220, 80, 255), 4.0f, 0, 1.5f);
        draw->AddText(ImVec2(screenCenter.x - 72, 26), IM_COL32(255, 255, 255, 255), "SOMALIA ESP ACTIVE");

        if (!s_DrawListTestLogged)
        {
            Logger::Log("[ESP] DrawList test=OK");
            s_DrawListTestLogged = true;
        }

        // 4. Verificacao de modulo SA-MP carregado e jogador vivo
        if (!SAMP::IsLoaded() || !RuntimeState::IsPlayerAlive())
            return;

        uint16_t localPlayerId = SAMP::GetLocalPlayerId();

        float localPos[3] = { 0.0f, 0.0f, 0.0f };
        SAMP::GetLocalPlayerPosition(localPos);

        // Renderizacao de Entidades do Mundo (Veiculos, Pickups, Objetos)
        RenderVehiclesESP(draw, localPos);
        RenderPickupsESP(draw, localPos);
        RenderObjectsESP(draw, localPos);

        bool shouldLog = (currentTick - s_LastLogTick >= 1000);

        int countValid = 0;
        int countStreamed = 0;
        int countPeds = 0;
        int countW2SOk = 0;
        int countDrawn = 0;
        bool sampleLogged = false;

        for (int i = 0; i < 1004; i++)
        {
            if (i == localPlayerId)
                continue;

            if (g_MenuState.visuals.enemyOnly && SAMP::IsTeammate(i))
                continue;

            SAMP::RemotePlayerData player;
            if (!SAMP::GetRemotePlayer(i, player) || !player.isValid)
            {
                s_PrevHealthArmor[i] = 0.0f;
                continue;
            }

            countValid++;

            if (!player.isStreamed)
                continue;

            countStreamed++;

            if (player.pGtaPed)
                countPeds++;

            float dx = player.position[0] - localPos[0];
            float dy = player.position[1] - localPos[1];
            float dz = player.position[2] - localPos[2];
            float distance = sqrtf(dx * dx + dy * dy + dz * dz);

            float head3D[3] = { player.position[0], player.position[1], player.position[2] + 0.85f };
            float feet3D[3] = { player.position[0], player.position[1], player.position[2] - 1.0f };

            if (player.pGtaPed)
            {
                float boneHead[3], boneFeet[3];
                if (GTA::GetPedBonePosition(player.pGtaPed, 8, boneHead))
                {
                    head3D[0] = boneHead[0];
                    head3D[1] = boneHead[1];
                    head3D[2] = boneHead[2] + 0.22f;
                }
                if (GTA::GetPedBonePosition(player.pGtaPed, 44, boneFeet))
                {
                    feet3D[0] = boneFeet[0];
                    feet3D[1] = boneFeet[1];
                    feet3D[2] = boneFeet[2] - 0.15f;
                }
            }

            // Monitoramento de dano
            float currentTotal = player.health + player.armor;
            if (s_PrevHealthArmor[i] > currentTotal && currentTotal >= 0.0f && s_PrevHealthArmor[i] > 0.0f)
            {
                float damageDealt = s_PrevHealthArmor[i] - currentTotal;
                if (damageDealt >= 1.0f && distance <= 160.0f)
                {
                    if (g_MenuState.visuals.hitmarker)
                    {
                        TriggerHitmarker();
                    }
                    if (g_MenuState.visuals.damageInformer)
                    {
                        AddDamageInformer(head3D[0], head3D[1], head3D[2], damageDealt);
                    }
                }
            }
            s_PrevHealthArmor[i] = currentTotal;

            if (distance > static_cast<float>(g_MenuState.visuals.maxDistance))
                continue;

            ImVec2 headScreen, feetScreen;
            bool w2sHead = WorldToScreen(head3D[0], head3D[1], head3D[2], headScreen);
            bool w2sFeet = WorldToScreen(feet3D[0], feet3D[1], feet3D[2], feetScreen);

            if (shouldLog && !sampleLogged)
            {
                Logger::Log("[ESP][PLAYER] id=%d ped=0x%p handle=0x%X position=(%.2f,%.2f,%.2f) health=%.1f armor=%.1f world=(%.2f,%.2f,%.2f) screen=(%.1f,%.1f) w2s=%s",
                    i, player.pGtaPed, player.gtaPedHandle,
                    player.position[0], player.position[1], player.position[2],
                    player.health, player.armor,
                    head3D[0], head3D[1], head3D[2],
                    headScreen.x, headScreen.y,
                    (w2sHead && w2sFeet) ? "true" : "false");
                sampleLogged = true;
            }

            if (!w2sHead || !w2sFeet)
                continue;

            countW2SOk++;

            float height = feetScreen.y - headScreen.y;
            if (height < 2.0f)
                continue;

            float width = height * 0.45f;
            ImVec2 boxMin(headScreen.x - width * 0.5f, headScreen.y);
            ImVec2 boxMax(headScreen.x + width * 0.5f, feetScreen.y);

            ImU32 boxColor = IM_COL32(255, 255, 255, 240);

            // A. 2D Box / Corner Box
            if (g_MenuState.visuals.boxESP)
            {
                if (g_MenuState.visuals.boxType == 0)
                {
                    draw->AddRect(ImVec2(boxMin.x - 1, boxMin.y - 1), ImVec2(boxMax.x + 1, boxMax.y + 1), IM_COL32(0, 0, 0, 220), 0.0f, 0, 1.0f);
                    draw->AddRect(boxMin, boxMax, boxColor, 0.0f, 0, 1.0f);
                    draw->AddRect(ImVec2(boxMin.x + 1, boxMin.y + 1), ImVec2(boxMax.x - 1, boxMax.y - 1), IM_COL32(0, 0, 0, 220), 0.0f, 0, 1.0f);
                }
                else
                {
                    DrawCornerBox(draw, boxMin, boxMax, boxColor, 1.2f);
                }
            }

            // B. Health Bar
            if (g_MenuState.visuals.healthESP)
            {
                DrawHealthBar(draw, boxMin, boxMax, player.health > 0.0f ? player.health : 100.0f, 100.0f);
            }

            // C. Armor Bar
            if (g_MenuState.visuals.armorESP && player.armor > 0.0f)
            {
                DrawArmorBar(draw, boxMin, boxMax, player.armor, 100.0f);
            }

            // D. Player Name & ID
            if (g_MenuState.visuals.nameESP)
            {
                char nameBuf[64];
                if (player.name[0] != '\0')
                    snprintf(nameBuf, sizeof(nameBuf), "%s [%d]", player.name, i);
                else
                    snprintf(nameBuf, sizeof(nameBuf), "Player [%d]", i);

                ImVec2 textSize = ImGui::CalcTextSize(nameBuf);
                ImVec2 textPos(headScreen.x - textSize.x * 0.5f, boxMin.y - textSize.y - 2.0f);

                draw->AddText(ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 255), nameBuf);
                draw->AddText(textPos, IM_COL32(255, 255, 255, 255), nameBuf);
            }

            // E. Distance Tag
            if (g_MenuState.visuals.distanceESP)
            {
                char distBuf[32];
                snprintf(distBuf, sizeof(distBuf), "%.1f m", distance);

                ImVec2 distSize = ImGui::CalcTextSize(distBuf);
                ImVec2 distPos(headScreen.x - distSize.x * 0.5f, boxMax.y + 2.0f);

                draw->AddText(ImVec2(distPos.x + 1, distPos.y + 1), IM_COL32(0, 0, 0, 255), distBuf);
                draw->AddText(distPos, IM_COL32(210, 210, 210, 255), distBuf);
            }

            // F. Skeleton / Bones
            if (g_MenuState.visuals.bonesESP && player.pGtaPed != nullptr)
            {
                DrawSkeleton(draw, player.pGtaPed, IM_COL32(240, 240, 240, 220));
            }

            // G. Snaplines
            if (g_MenuState.visuals.snaplines)
            {
                ImVec2 lineOrigin = (g_MenuState.visuals.snaplineOrigin == 0) ?
                    ImVec2(screenCenter.x, displaySize.y) : screenCenter;

                draw->AddLine(lineOrigin, feetScreen, IM_COL32(137, 207, 240, 180), 1.0f);
            }

            countDrawn++;
        }

        if (shouldLog)
        {
            Logger::Log("[ESP] players_total=1004 valid=%d streamed=%d peds=%d w2s_ok=%d drawn=%d",
                countValid, countStreamed, countPeds, countW2SOk, countDrawn);
            s_LastLogTick = currentTick;
        }
    }
}
