#include "TargetSelector.h"
#include "Aimbot.h"
#include "../../Engine/SAMP/SAMP.h"
#include "../../Engine/GTA/GTA.h"
#include "../../Core/RuntimeState.h"
#include "../Visuals/ESP.h"
#include <math.h>
#include <string.h>

namespace TargetSelector
{
    TargetInfo FindBestTarget(const WeaponAimConfig& config, ImVec2 screenCenter, float fovRadius, int& outCandidates, int& outInsideFov, bool isRage)
    {
        TargetInfo bestTarget = {};
        bestTarget.valid = false;
        outCandidates = 0;
        outInsideFov = 0;

        if (!RuntimeState::IsPlayerAlive())
            return bestTarget;

        if (!SAMP::IsLoaded())
            return bestTarget;

        uint16_t localPlayerId = SAMP::GetLocalPlayerId();
        float localPos[3] = { 0.0f, 0.0f, 0.0f };
        SAMP::GetLocalPlayerPosition(localPos);

        float bestMetric = 999999.0f;

        // Mapeamento dos ossos reais do GTA San Andreas 1.0 US
        int configuredBone = config.bone;
        if (!isRage && g_MenuState.legitBot.preferBodyAim && configuredBone == 0)
        {
            // Se preferBodyAim estiver ativo no LegitBot e a arma configurada para HEAD, prefere CHEST
            configuredBone = 2;
        }

        int targetBoneId = 8;
        const char* targetBoneName = "HEAD";
        switch (configuredBone)
        {
        case 0:
            targetBoneId = 8;
            targetBoneName = "HEAD";
            break;
        case 1:
            targetBoneId = 5;
            targetBoneName = "NECK";
            break;
        case 2:
            targetBoneId = 4;
            targetBoneName = "CHEST";
            break;
        case 3:
            targetBoneId = 2;
            targetBoneName = "PELVIS";
            break;
        default:
            targetBoneId = 8;
            targetBoneName = "HEAD";
            break;
        }

        for (int i = 0; i < 1004; i++)
        {
            if (i == localPlayerId)
                continue;

            if (config.teamCheck && SAMP::IsTeammate(i))
                continue;

            SAMP::RemotePlayerData player;
            if (!SAMP::GetRemotePlayer(i, player) || !player.isValid)
                continue;

            if (!player.isStreamed || !player.pGtaPed)
                continue;

            outCandidates++;

            // 1. Filtro: Ignore Dead
            if (config.ignoreDead)
            {
                if (!GTA::IsPedAlive(player.pGtaPed) || player.health <= 0.0f)
                    continue;
            }

            // 2. Filtro: Max Distance (Distância 3D Euclidiana)
            float dx = player.position[0] - localPos[0];
            float dy = player.position[1] - localPos[1];
            float dz = player.position[2] - localPos[2];
            float dist3D = sqrtf(dx * dx + dy * dy + dz * dz);

            if (dist3D > config.maxDistance)
                continue;

            // 3. Obtenção do Osso Selecionado via API nativa do GTA
            int effectiveBoneId = targetBoneId;
            const char* effectiveBoneName = targetBoneName;

            if (!isRage && g_MenuState.legitBot.ignoreLimbs && player.pGtaPed)
            {
                float* pVelX = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player.pGtaPed) + 0x44);
                float* pVelY = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player.pGtaPed) + 0x48);
                if (pVelX && pVelY)
                {
                    float speed = sqrtf((*pVelX) * (*pVelX) + (*pVelY) * (*pVelY));
                    if (speed > 0.05f)
                    {
                        // Se o alvo estiver correndo e o osso não for cabeça/pescoço/peito/pelve, força CHEST
                        if (effectiveBoneId != 8 && effectiveBoneId != 5 && effectiveBoneId != 4 && effectiveBoneId != 2)
                        {
                            effectiveBoneId = 4;
                            effectiveBoneName = "CHEST";
                        }
                    }
                }
            }

            float boneWorldPos[3] = { 0.0f, 0.0f, 0.0f };
            if (!GTA::GetPedBonePosition(player.pGtaPed, effectiveBoneId, boneWorldPos))
            {
                // Fallback proporcional caso a interpolação de skin do osso não responda
                boneWorldPos[0] = player.position[0];
                boneWorldPos[1] = player.position[1];
                boneWorldPos[2] = player.position[2] + (configuredBone == 0 ? 0.8f : (configuredBone == 1 ? 0.6f : 0.2f));
            }

            // 4. Filtro: Visibility Check (Raycast seguro GTA SA CWorld::GetIsLineOfSightClear 0x0056A490)
            if (config.visibilityCheck)
            {
                bool isVisible = true;
                __try
                {
                    typedef bool(__cdecl* tGetIsLineOfSightClear)(float*, float*, bool, bool, bool, bool, bool, bool, bool);
                    auto fnLOS = reinterpret_cast<tGetIsLineOfSightClear>(0x0056A490);
                    if (fnLOS)
                    {
                        float camPos[3];
                        if (!GTA::GetCameraPosition(camPos))
                        {
                            camPos[0] = localPos[0];
                            camPos[1] = localPos[1];
                            camPos[2] = localPos[2] + 0.7f;
                        }
                        // buildings=true, vehicles=false, peds=false, objects=true, dummies=false
                        isVisible = fnLOS(camPos, boneWorldPos, true, false, false, true, false, false, false);
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    isVisible = true;
                }

                if (!isVisible)
                    continue;
            }

            // 5. Projeção de Tela via WorldToScreen existente do ESP
            ImVec2 boneScreen;
            if (!ESP::WorldToScreen(boneWorldPos[0], boneWorldPos[1], boneWorldPos[2], boneScreen))
                continue;

            // 6. Distância em Pixels do Alvo até o Centro da Tela (Crosshair)
            float screenDx = boneScreen.x - screenCenter.x;
            float screenDy = boneScreen.y - screenCenter.y;
            float distanceFromCrosshair = sqrtf(screenDx * screenDx + screenDy * screenDy);

            // 7. Checagem de FOV Real
            if (distanceFromCrosshair > fovRadius)
                continue;

            outInsideFov++;

            // 8. Aplicação de Prioridade de Seleção
            float currentMetric = 0.0f;
            switch (config.priority)
            {
            case 0: // Closest To Crosshair
                currentMetric = distanceFromCrosshair;
                break;
            case 1: // Closest Distance 3D
                currentMetric = dist3D;
                break;
            case 2: // Lowest Health
                currentMetric = (player.health > 0.0f) ? player.health : 100.0f;
                break;
            default:
                currentMetric = distanceFromCrosshair;
                break;
            }

            // Histerese / Sticky Target: bônus de 20% para o alvo já selecionado no frame anterior
            // para evitar oscilações contínuas quando alvos cruzam o FOV
            int prevTargetId = Aimbot::GetCurrentTarget().valid ? Aimbot::GetCurrentTarget().playerId : -1;
            if (i == prevTargetId && prevTargetId != -1)
            {
                currentMetric *= 0.80f;
            }

            if (currentMetric < bestMetric)
            {
                bestMetric = currentMetric;
                bestTarget.valid = true;
                bestTarget.playerId = i;
                bestTarget.ped = player.pGtaPed;
                bestTarget.screenPosition = boneScreen;
                bestTarget.worldPosition[0] = boneWorldPos[0];
                bestTarget.worldPosition[1] = boneWorldPos[1];
                bestTarget.worldPosition[2] = boneWorldPos[2];
                bestTarget.distance3D = dist3D;
                bestTarget.distanceFromCrosshair = distanceFromCrosshair;
                bestTarget.bone = effectiveBoneId;
                bestTarget.boneName = effectiveBoneName;
                bestTarget.health = player.health;
                bestTarget.armor = player.armor;
                strncpy(bestTarget.name, player.name[0] ? player.name : "Player", sizeof(bestTarget.name) - 1);
            }
        }

        return bestTarget;
    }
}
