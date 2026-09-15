#include "ESP.h"
#include "../../Config/Config.h"
#include "../../UI/Theme.h"
#include "../../Engine/SAMP/SAMP.h"
#include "../../Engine/GTA/GTA.h"
#include "../../Core/Logger.h"
#include "../../Core/RuntimeState.h"
#include "../AntiAim/AntiAim.h"
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

    // FPS Counter
    static int s_FpsCount = 0;
    static int s_FpsDisplay = 0;
    static ULONGLONG s_FpsLastTick = 0;

    // Weapon Name Lookup (GTA SA weapon IDs)
    static const char* GetWeaponName(uint32_t weaponId)
    {
        switch (weaponId)
        {
        case 0:  return "Fist";
        case 1:  return "Brass Knuckles";
        case 4:  return "Knife";
        case 5:  return "Bat";
        case 8:  return "Katana";
        case 9:  return "Chainsaw";
        case 22: return "Colt 45";
        case 23: return "Silenced";
        case 24: return "Deagle";
        case 25: return "Shotgun";
        case 26: return "Sawnoff";
        case 27: return "SPAS-12";
        case 28: return "Uzi";
        case 29: return "MP5";
        case 30: return "AK-47";
        case 31: return "M4";
        case 32: return "Tec-9";
        case 33: return "Country";
        case 34: return "Sniper";
        case 35: return "RPG";
        case 36: return "HS Rocket";
        case 37: return "Flamethrower";
        case 38: return "Minigun";
        case 41: return "Spray Can";
        case 42: return "Fire Ext.";
        case 46: return "Parachute";
        default: return nullptr;
        }
    }

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

        // Outline preto sutil para contraste em qualquer ambiente
        float ot = thickness + 1.2f;
        ImU32 shadow = IM_COL32(0, 0, 0, 200);

        // Top Left
        draw->AddLine(min, ImVec2(min.x + lineW, min.y), shadow, ot);
        draw->AddLine(min, ImVec2(min.x, min.y + lineH), shadow, ot);
        draw->AddLine(min, ImVec2(min.x + lineW, min.y), color, thickness);
        draw->AddLine(min, ImVec2(min.x, min.y + lineH), color, thickness);

        // Top Right
        draw->AddLine(ImVec2(max.x, min.y), ImVec2(max.x - lineW, min.y), shadow, ot);
        draw->AddLine(ImVec2(max.x, min.y), ImVec2(max.x, min.y + lineH), shadow, ot);
        draw->AddLine(ImVec2(max.x, min.y), ImVec2(max.x - lineW, min.y), color, thickness);
        draw->AddLine(ImVec2(max.x, min.y), ImVec2(max.x, min.y + lineH), color, thickness);

        // Bottom Left
        draw->AddLine(ImVec2(min.x, max.y), ImVec2(min.x + lineW, max.y), shadow, ot);
        draw->AddLine(ImVec2(min.x, max.y), ImVec2(min.x, max.y - lineH), shadow, ot);
        draw->AddLine(ImVec2(min.x, max.y), ImVec2(min.x + lineW, max.y), color, thickness);
        draw->AddLine(ImVec2(min.x, max.y), ImVec2(min.x, max.y - lineH), color, thickness);

        // Bottom Right
        draw->AddLine(max, ImVec2(max.x - lineW, max.y), shadow, ot);
        draw->AddLine(max, ImVec2(max.x, max.y - lineH), shadow, ot);
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

        draw->AddLine(s1, s2, IM_COL32(0, 0, 0, 200), 2.2f);
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

    static ImU32 GetPlayerOrgColor(uint32_t sampColor, bool isAlly)
    {
        if (sampColor == 0)
        {
            return isAlly ? IM_COL32(100, 255, 100, 255) : IM_COL32(235, 235, 235, 255);
        }

        uint8_t r = static_cast<uint8_t>((sampColor >> 16) & 0xFF);
        uint8_t g = static_cast<uint8_t>((sampColor >> 8) & 0xFF);
        uint8_t b = static_cast<uint8_t>(sampColor & 0xFF);

        // Se a cor for muito escura (quase preta), garante brilho mínimo para leitura no ESP
        if (r < 60 && g < 60 && b < 60)
        {
            if (r < 160) r = 160;
            if (g < 160) g = 160;
            if (b < 160) b = 160;
        }

        return IM_COL32(r, g, b, 255);
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

        // 1. Render FOV Circle (somente com menu fechado para nao atravessar a UI)
        if (g_MenuState.visuals.drawFOVCircle && !g_MenuState.menuOpen)
        {
            float radius = static_cast<float>(g_MenuState.visuals.fovCircleRadius) * 4.0f;
            draw->AddCircle(screenCenter, radius, IM_COL32(int(accent_colour[0] * 255), int(accent_colour[1] * 255), int(accent_colour[2] * 255), 180), 64, 1.2f);
        }

        // 1.1 Custom Screen Crosshair (somente com menu fechado)
        if (g_MenuState.visuals.customCrosshair && !g_MenuState.menuOpen)
        {
            float crossLen = 6.0f;
            float crossGap = 3.0f;
            draw->AddCircleFilled(screenCenter, 1.5f, IM_COL32(255, 60, 90, 255));
            draw->AddLine(ImVec2(screenCenter.x - crossGap - crossLen, screenCenter.y), ImVec2(screenCenter.x - crossGap, screenCenter.y), IM_COL32(255, 60, 90, 240), 1.5f);
            draw->AddLine(ImVec2(screenCenter.x + crossGap, screenCenter.y), ImVec2(screenCenter.x + crossGap + crossLen, screenCenter.y), IM_COL32(255, 60, 90, 240), 1.5f);
            draw->AddLine(ImVec2(screenCenter.x, screenCenter.y - crossGap - crossLen), ImVec2(screenCenter.x, screenCenter.y - crossGap), IM_COL32(255, 60, 90, 240), 1.5f);
            draw->AddLine(ImVec2(screenCenter.x, screenCenter.y + crossGap), ImVec2(screenCenter.x, screenCenter.y + crossGap + crossLen), IM_COL32(255, 60, 90, 240), 1.5f);
        }

        // 1.15 Show FPS / Ping Overlay
        float hudY = 20.0f;
        if (g_MenuState.visuals.showFPS)
        {
            s_FpsCount++;
            if (currentTick - s_FpsLastTick >= 1000)
            {
                s_FpsDisplay = s_FpsCount;
                s_FpsCount = 0;
                s_FpsLastTick = currentTick;
            }

            char fpsBuf[64];
            snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %d", s_FpsDisplay);
            ImVec2 fpsSz = ImGui::CalcTextSize(fpsBuf);
            float fpsPadX = 8.0f, fpsPadY = 4.0f;
            draw->AddRectFilled(ImVec2(20, hudY), ImVec2(20 + fpsSz.x + fpsPadX * 2, hudY + fpsSz.y + fpsPadY * 2), IM_COL32(15, 15, 20, 200), 4.0f);
            draw->AddRect(ImVec2(20, hudY), ImVec2(20 + fpsSz.x + fpsPadX * 2, hudY + fpsSz.y + fpsPadY * 2), IM_COL32(50, 50, 60, 200), 4.0f);
            draw->AddText(ImVec2(20 + fpsPadX, hudY + fpsPadY), IM_COL32(255, 255, 255, 230), fpsBuf);
            hudY += fpsSz.y + fpsPadY * 2 + 6.0f;
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
            float cardW = 215.0f, cardH = 26.0f;
            draw->AddRectFilled(ImVec2(20, hudY), ImVec2(20 + cardW, hudY + cardH), IM_COL32(20, 20, 20, 215), 4.0f);
            draw->AddRect(ImVec2(20, hudY), ImVec2(20 + cardW, hudY + cardH), IM_COL32(int(accent_colour[0] * 255), int(accent_colour[1] * 255), int(accent_colour[2] * 255), 220), 4.0f, 0, 1.2f);
            draw->AddText(ImVec2(28, hudY + 5), IM_COL32(int(accent_colour[0] * 255), int(accent_colour[1] * 255), int(accent_colour[2] * 255), 255), "INVERTEBRED: ATIVO (TWIST)");
            hudY += cardH + 6.0f;
        }

        // 1.45 Indicador Desync Angles em Execucao
        if (g_MenuState.antiAim.desync || (g_MenuState.antiAim.enabled && AntiAim::IsActive()))
        {
            float realAng = AntiAim::GetRealAngle();
            float fakeAng = AntiAim::GetFakeAngle();
            float delta = fabsf(fakeAng - realAng);
            if (delta > 180.0f) delta = 360.0f - delta;

            char desyncBuf[128];
            snprintf(desyncBuf, sizeof(desyncBuf), "DESYNC ANGLES: ATIVO (R: %.0f | F: %.0f | D: %.0f)", realAng, fakeAng, delta);
            ImVec2 txtSz = ImGui::CalcTextSize(desyncBuf);
            float cardW = txtSz.x + 18.0f;
            float cardH = 26.0f;

            draw->AddRectFilled(ImVec2(20, hudY), ImVec2(20 + cardW, hudY + cardH), IM_COL32(20, 20, 20, 215), 4.0f);
            draw->AddRect(ImVec2(20, hudY), ImVec2(20 + cardW, hudY + cardH), IM_COL32(255, 165, 0, 220), 4.0f, 0, 1.2f);
            draw->AddText(ImVec2(28, hudY + 5), IM_COL32(255, 200, 50, 255), desyncBuf);
            hudY += cardH + 6.0f;
        }

        // 1.46 Visualizador 3D de Desync no chão sob o jogador local (Terceira Pessoa)
        if (RuntimeState::IsPlayerAlive() && (g_MenuState.antiAim.desync || (g_MenuState.antiAim.enabled && AntiAim::IsActive())))
        {
            float localPos[3] = { 0 };
            if (SAMP::GetLocalPlayerPosition(localPos))
            {
                float realRad = AntiAim::GetRealAngle() * (3.14159265f / 180.0f);
                float fakeRad = AntiAim::GetFakeAngle() * (3.14159265f / 180.0f);

                float origin3D[3] = { localPos[0], localPos[1], localPos[2] - 0.95f };
                // Vetor Real (Verde): onde o jogador realmente está olhando
                float realTip3D[3] = { origin3D[0] - sinf(realRad) * 1.2f, origin3D[1] + cosf(realRad) * 1.2f, origin3D[2] };
                // Vetor Fake / Desync (Laranja): o que o servidor e inimigos vêem
                float fakeTip3D[3] = { origin3D[0] - sinf(fakeRad) * 1.2f, origin3D[1] + cosf(fakeRad) * 1.2f, origin3D[2] };

                ImVec2 originScr, realScr, fakeScr;
                if (WorldToScreen(origin3D[0], origin3D[1], origin3D[2], originScr))
                {
                    if (WorldToScreen(realTip3D[0], realTip3D[1], realTip3D[2], realScr))
                    {
                        draw->AddLine(originScr, realScr, IM_COL32(50, 255, 50, 220), 2.5f);
                        draw->AddCircleFilled(realScr, 3.5f, IM_COL32(50, 255, 50, 255));
                    }
                    if (WorldToScreen(fakeTip3D[0], fakeTip3D[1], fakeTip3D[2], fakeScr))
                    {
                        draw->AddLine(originScr, fakeScr, IM_COL32(255, 140, 0, 220), 2.5f);
                        draw->AddCircleFilled(fakeScr, 3.5f, IM_COL32(255, 140, 0, 255));
                    }
                }
            }
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

            bool isAlly = SAMP::IsTeammate(i);
            ImU32 orgColor = GetPlayerOrgColor(player.color, isAlly);
            ImU32 boxColor = orgColor;

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

            // D. Player Name & ID (Colorido com a cor da Organizacao / Faccao)
            if (g_MenuState.visuals.nameESP)
            {
                char nameBuf[64];
                if (player.name[0] != '\0')
                    snprintf(nameBuf, sizeof(nameBuf), "%s [%d]", player.name, i);
                else
                    snprintf(nameBuf, sizeof(nameBuf), "Player [%d]", i);

                ImVec2 textSize = ImGui::CalcTextSize(nameBuf);
                ImVec2 textPos(headScreen.x - textSize.x * 0.5f, boxMin.y - textSize.y - 2.0f);

                // Contorno 4-direcional preto para contraste maximo em qualquer fundo
                draw->AddText(ImVec2(textPos.x + 1, textPos.y), IM_COL32(0, 0, 0, 255), nameBuf);
                draw->AddText(ImVec2(textPos.x - 1, textPos.y), IM_COL32(0, 0, 0, 255), nameBuf);
                draw->AddText(ImVec2(textPos.x, textPos.y + 1), IM_COL32(0, 0, 0, 255), nameBuf);
                draw->AddText(ImVec2(textPos.x, textPos.y - 1), IM_COL32(0, 0, 0, 255), nameBuf);

                // Nome com a cor oficial da organizacao
                draw->AddText(textPos, orgColor, nameBuf);
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

            // E2. Weapon Name ESP
            if (g_MenuState.visuals.weaponESP && player.pGtaPed != nullptr)
            {
                __try
                {
                    uintptr_t pedA = reinterpret_cast<uintptr_t>(player.pGtaPed);
                    uint8_t slot = *reinterpret_cast<uint8_t*>(pedA + 0x718);
                    uintptr_t weapPtr = pedA + 0x5A0 + slot * 0x1C;
                    uint32_t weapType = *reinterpret_cast<uint32_t*>(weapPtr);

                    if (weapType > 0)
                    {
                        const char* weapName = GetWeaponName(weapType);
                        char weapBuf[32];
                        if (weapName)
                            snprintf(weapBuf, sizeof(weapBuf), "%s", weapName);
                        else
                            snprintf(weapBuf, sizeof(weapBuf), "Weapon [%d]", weapType);

                        ImVec2 wSz = ImGui::CalcTextSize(weapBuf);
                        float weapY = boxMax.y + 2.0f;
                        if (g_MenuState.visuals.distanceESP)
                        {
                            char tmpDist[32];
                            snprintf(tmpDist, sizeof(tmpDist), "%.1f m", distance);
                            weapY += ImGui::CalcTextSize(tmpDist).y + 2.0f;
                        }
                        ImVec2 wPos(headScreen.x - wSz.x * 0.5f, weapY);

                        draw->AddText(ImVec2(wPos.x + 1, wPos.y + 1), IM_COL32(0, 0, 0, 255), weapBuf);
                        draw->AddText(wPos, IM_COL32(255, 180, 60, 230), weapBuf);
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {}
            }

            // F. Skeleton / Bones
            if (g_MenuState.visuals.bonesESP && player.pGtaPed != nullptr)
            {
                DrawSkeleton(draw, player.pGtaPed, orgColor);
            }

            // G. Snaplines
            if (g_MenuState.visuals.snaplines)
            {
                ImVec2 lineOrigin = (g_MenuState.visuals.snaplineOrigin == 0) ?
                    ImVec2(screenCenter.x, displaySize.y) : screenCenter;

                draw->AddLine(lineOrigin, feetScreen, IM_COL32(int(accent_colour[0] * 255), int(accent_colour[1] * 255), int(accent_colour[2] * 255), 180), 1.0f);
            }

            // H. Line of Sight (Look Direction)
            if (g_MenuState.visuals.lineOfSight && player.pGtaPed != nullptr)
            {
                __try
                {
                    uintptr_t pedA = reinterpret_cast<uintptr_t>(player.pGtaPed);
                    uintptr_t pMatrix = *reinterpret_cast<uintptr_t*>(pedA + 0x14);
                    if (pMatrix)
                    {
                        float fwdX = *reinterpret_cast<float*>(pMatrix + 0x10);
                        float fwdY = *reinterpret_cast<float*>(pMatrix + 0x14);

                        float losEndX = head3D[0] + fwdX * 5.0f;
                        float losEndY = head3D[1] + fwdY * 5.0f;
                        float losEndZ = head3D[2];

                        ImVec2 losScreen;
                        if (WorldToScreen(losEndX, losEndY, losEndZ, losScreen))
                        {
                            draw->AddLine(headScreen, losScreen, IM_COL32(255, 255, 0, 120), 1.0f);
                        }
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {}
            }

            // I. Target Highlight (Pulsing glow on closest-to-crosshair player)
            if (g_MenuState.visuals.targetHighlight)
            {
                float dxCross = headScreen.x - screenCenter.x;
                float dyCross = headScreen.y - screenCenter.y;
                float crossDist = sqrtf(dxCross * dxCross + dyCross * dyCross);
                float fovRadius = static_cast<float>(g_MenuState.visuals.fovCircleRadius) * 4.0f;

                if (crossDist <= fovRadius)
                {
                    float pulse = (sinf(static_cast<float>(currentTick) * 0.005f) + 1.0f) * 0.5f;
                    int pulseAlpha = static_cast<int>(40.0f + pulse * 50.0f);
                    draw->AddRectFilled(ImVec2(boxMin.x - 2, boxMin.y - 2), ImVec2(boxMax.x + 2, boxMax.y + 2), IM_COL32(255, 200, 60, pulseAlpha), 2.0f);
                }
            }

            countDrawn++;
        }

        // J. Off-screen Arrows (rendered after main loop for players NOT on screen)
        if (g_MenuState.visuals.offscreenArrows && SAMP::IsLoaded() && RuntimeState::IsPlayerAlive())
        {
            float arrowMargin = 30.0f;
            float arrowSize = 10.0f;
            uint16_t localId = SAMP::GetLocalPlayerId();

            for (int i = 0; i < 1004; i++)
            {
                if (i == localId) continue;

                SAMP::RemotePlayerData rp;
                if (!SAMP::GetRemotePlayer(i, rp) || !rp.isValid || !rp.isStreamed) continue;
                if (g_MenuState.visuals.enemyOnly && SAMP::IsTeammate(i)) continue;

                float dx = rp.position[0] - localPos[0];
                float dy = rp.position[1] - localPos[1];
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist > static_cast<float>(g_MenuState.visuals.maxDistance)) continue;

                ImVec2 testScreen;
                if (WorldToScreen(rp.position[0], rp.position[1], rp.position[2], testScreen))
                {
                    if (testScreen.x >= 0 && testScreen.x <= displaySize.x && testScreen.y >= 0 && testScreen.y <= displaySize.y)
                        continue; // On screen, skip
                }

                // Calculate direction from center to target
                float dirX = rp.position[0] - localPos[0];
                float dirY = rp.position[1] - localPos[1];
                float angle = atan2f(dirY, dirX);

                // Project arrow position at screen edge
                float halfW = displaySize.x * 0.5f - arrowMargin;
                float halfH = displaySize.y * 0.5f - arrowMargin;

                float cosA = cosf(angle);
                float sinA = sinf(angle);

                float scale = 99999.0f;
                if (fabsf(cosA) > 0.001f)
                    scale = ImMin(scale, halfW / fabsf(cosA));
                if (fabsf(sinA) > 0.001f)
                    scale = ImMin(scale, halfH / fabsf(sinA));

                float arrowX = screenCenter.x + cosA * scale;
                float arrowY = screenCenter.y - sinA * scale;

                arrowX = ImClamp(arrowX, arrowMargin, displaySize.x - arrowMargin);
                arrowY = ImClamp(arrowY, arrowMargin, displaySize.y - arrowMargin);

                // Draw triangle arrow pointing toward enemy
                float perpX = -sinA;
                float perpY = cosA;
                ImVec2 tip(arrowX + cosA * arrowSize, arrowY - sinA * arrowSize);
                ImVec2 left(arrowX + perpX * arrowSize * 0.5f, arrowY - perpY * arrowSize * 0.5f);
                ImVec2 right(arrowX - perpX * arrowSize * 0.5f, arrowY + perpY * arrowSize * 0.5f);

                bool isAllyOff = SAMP::IsTeammate(i);
                ImU32 arrowCol = (GetPlayerOrgColor(rp.color, isAllyOff) & 0x00FFFFFF) | 0xC8000000;
                draw->AddTriangleFilled(tip, left, right, arrowCol);
            }
        }

        if (shouldLog)
        {
            Logger::Log("[ESP] players_total=1004 valid=%d streamed=%d peds=%d w2s_ok=%d drawn=%d",
                countValid, countStreamed, countPeds, countW2SOk, countDrawn);
            s_LastLogTick = currentTick;
        }
    }
}
