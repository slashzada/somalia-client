#pragma once
#include <windows.h>
#include <stdint.h>
#include "../../Render/ImGui/imgui.h"
#include "../../Config/Config.h"

struct TargetInfo
{
    bool valid = false;
    int playerId = -1;
    void* ped = nullptr;
    ImVec2 screenPosition = ImVec2(0, 0);
    float worldPosition[3] = { 0, 0, 0 };
    float distance3D = 0.0f;
    float distanceFromCrosshair = 0.0f;
    int bone = 8;
    const char* boneName = "HEAD";
    float health = 0.0f;
    float armor = 0.0f;
    char name[32] = { 0 };
};

namespace TargetSelector
{
    TargetInfo FindBestTarget(const WeaponAimConfig& config, ImVec2 screenCenter, float fovRadius, int& outCandidates, int& outInsideFov, bool isRage = false);
}
