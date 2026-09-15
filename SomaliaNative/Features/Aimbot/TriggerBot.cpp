#include "TriggerBot.h"
#include "../../Config/Config.h"
#include "../../Engine/GTA/GTA.h"
#include "../../Engine/SAMP/SAMP.h"
#include "../../Core/RuntimeState.h"
#include "../Visuals/ESP.h"
#include <math.h>

namespace TriggerBot
{
    static ULONGLONG s_TargetAcquiredTick = 0;
    static ULONGLONG s_LastFireTick = 0;
    static ULONGLONG s_ClickPressTick = 0;
    static bool s_NeedReleaseLMB = false;

    static float DistPointToSegment(ImVec2 p, ImVec2 a, ImVec2 b)
    {
        float abx = b.x - a.x;
        float aby = b.y - a.y;
        float apx = p.x - a.x;
        float apy = p.y - a.y;
        float abLenSq = abx * abx + aby * aby;
        if (abLenSq < 1e-4f)
        {
            float dx = p.x - a.x;
            float dy = p.y - a.y;
            return sqrtf(dx * dx + dy * dy);
        }
        float t = (apx * abx + apy * aby) / abLenSq;
        if (t < 0.0f) t = 0.0f;
        else if (t > 1.0f) t = 1.0f;
        float projX = a.x + t * abx;
        float projY = a.y + t * aby;
        float dx = p.x - projX;
        float dy = p.y - projY;
        return sqrtf(dx * dx + dy * dy);
    }

    void Initialize()
    {
        Reset();
    }

    void Reset()
    {
        if (s_NeedReleaseLMB)
        {
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            s_NeedReleaseLMB = false;
        }
        s_TargetAcquiredTick = 0;
        s_ClickPressTick = 0;
    }

    bool IsTargetUnderCrosshair(int& outTargetPlayerId)
    {
        outTargetPlayerId = -1;

        if (!RuntimeState::IsPlayerAlive())
            return false;

        if (!SAMP::IsLoaded())
            return false;

        uintptr_t pPlayerPool = SAMP::GetPlayerPool();
        if (!pPlayerPool)
            return false;

        uint16_t localPlayerId = SAMP::GetLocalPlayerId();
        int maxPlayerId = SAMP::GetLargestPlayerId();
        uintptr_t isListedOffset = SAMP::GetIsListedOffset();

        float localPos[3] = { 0.0f, 0.0f, 0.0f };
        SAMP::GetLocalPlayerPosition(localPos);

        ImVec2 crosshair = GTA::GetCrosshairScreenPos();
        if (crosshair.x <= 0.0f || crosshair.y <= 0.0f)
            return false;

        for (int i = 0; i <= maxPlayerId; i++)
        {
            if (i == localPlayerId)
                continue;

            int isListed = 0;
            __try
            {
                isListed = *reinterpret_cast<int*>(pPlayerPool + isListedOffset + i * 4);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { isListed = 0; }

            if (isListed != 1)
                continue;

            if (SAMP::IsTeammate(i))
                continue;

            SAMP::RemotePlayerData player;
            if (!SAMP::GetRemotePlayer(i, player, pPlayerPool) || !player.isValid)
                continue;

            if (!player.isStreamed || !player.pGtaPed)
                continue;

            if (!GTA::IsPedAlive(player.pGtaPed) || player.health <= 0.0f)
                continue;

            // Distância Euclidiana 3D
            float dx = player.position[0] - localPos[0];
            float dy = player.position[1] - localPos[1];
            float dz = player.position[2] - localPos[2];
            float dist3D = sqrtf(dx * dx + dy * dy + dz * dz);
            if (dist3D > 180.0f)
                continue;

            // Obtenção dos ossos para projeção da silhueta
            float headPos[3] = { 0.0f, 0.0f, 0.0f };
            float feetPos[3] = { 0.0f, 0.0f, 0.0f };
            float pelvisPos[3] = { 0.0f, 0.0f, 0.0f };

            if (!GTA::GetPedBonePosition(player.pGtaPed, 8, headPos))
            {
                headPos[0] = player.position[0];
                headPos[1] = player.position[1];
                headPos[2] = player.position[2] + 0.85f;
            }
            else
            {
                headPos[2] += 0.12f;
            }

            if (!GTA::GetPedBonePosition(player.pGtaPed, 44, feetPos))
            {
                feetPos[0] = player.position[0];
                feetPos[1] = player.position[1];
                feetPos[2] = player.position[2] - 1.0f;
            }

            if (!GTA::GetPedBonePosition(player.pGtaPed, 2, pelvisPos))
            {
                pelvisPos[0] = player.position[0];
                pelvisPos[1] = player.position[1];
                pelvisPos[2] = player.position[2];
            }

            ImVec2 headScreen, feetScreen, pelvisScreen;
            if (!ESP::WorldToScreen(headPos[0], headPos[1], headPos[2], headScreen))
                continue;
            if (!ESP::WorldToScreen(feetPos[0], feetPos[1], feetPos[2], feetScreen))
                continue;

            bool hasPelvis = ESP::WorldToScreen(pelvisPos[0], pelvisPos[1], pelvisPos[2], pelvisScreen);

            float height = feetScreen.y - headScreen.y;
            if (height < 2.0f)
                continue;

            // Teste geométrico: retícula sobre a cabeça, tronco ou membros
            // Inclui tolerância de raio da retícula do GTA (12px) para disparo no instante exato do contato visual
            const float reticleTolerance = 12.0f;
            float headRadius = height * 0.15f + reticleTolerance;
            float torsoRadius = height * 0.20f + reticleTolerance;
            float legsRadius = height * 0.15f + reticleTolerance;

            float dxHead = crosshair.x - headScreen.x;
            float dyHead = crosshair.y - headScreen.y;
            bool hitHead = (dxHead * dxHead + dyHead * dyHead) <= (headRadius * headRadius);

            bool hitTorso = false;
            bool hitLegs = false;
            if (hasPelvis)
            {
                hitTorso = (DistPointToSegment(crosshair, headScreen, pelvisScreen) <= torsoRadius);
                hitLegs = (DistPointToSegment(crosshair, pelvisScreen, feetScreen) <= legsRadius);
            }
            else
            {
                hitTorso = (DistPointToSegment(crosshair, headScreen, feetScreen) <= torsoRadius);
            }

            float boxHalfWidth = (height * 0.24f) + reticleTolerance;
            float centerX = hasPelvis ? pelvisScreen.x : headScreen.x;
            bool hitBox = (crosshair.x >= centerX - boxHalfWidth &&
                           crosshair.x <= centerX + boxHalfWidth &&
                           crosshair.y >= headScreen.y - headRadius &&
                           crosshair.y <= feetScreen.y + 4.0f);

            if (!hitHead && !hitTorso && !hitLegs && !hitBox)
                continue;

            // Verificação de visibilidade (Line of Sight)
            bool isVisible = true;
            __try
            {
                typedef bool(__cdecl* tGetIsLineOfSightClear)(float*, float*, bool, bool, bool, bool, bool, bool, bool);
                auto fnLOS = reinterpret_cast<tGetIsLineOfSightClear>(0x0056A490);
                if (fnLOS)
                {
                    float camPos[3] = { localPos[0], localPos[1], localPos[2] + 0.7f };
                    isVisible = fnLOS(camPos, headPos, true, false, false, true, false, false, false);
                    if (!isVisible)
                    {
                        isVisible = fnLOS(camPos, pelvisPos, true, false, false, true, false, false, false);
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                isVisible = true;
            }

            if (!isVisible)
                continue;

            outTargetPlayerId = i;
            return true;
        }

        return false;
    }

    void Update()
    {
        ULONGLONG now = GetTickCount64();

        // 1. Liberação do clique esquerdo com intervalo ideal (25ms) para garantir registro no GTA SA
        if (s_NeedReleaseLMB && (now - s_ClickPressTick >= 25))
        {
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            s_NeedReleaseLMB = false;
        }

        // 2. Checagem de ativação do recurso e menu
        if (!g_MenuState.triggerBot.enabled || g_MenuState.menuOpen)
        {
            s_TargetAcquiredTick = 0;
            return;
        }

        // 3. Foco da janela do GTA San Andreas
        HWND gtaHwnd = GTA::GetWindowHandle();
        if (gtaHwnd && GetForegroundWindow() != gtaHwnd)
        {
            s_TargetAcquiredTick = 0;
            return;
        }

        // 4. Jogador local deve estar vivo
        if (!RuntimeState::IsPlayerAlive())
        {
            s_TargetAcquiredTick = 0;
            return;
        }

        // 5. Verificação se o usuário está mirando (RMB)
        bool isAiming = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        if (!isAiming)
        {
            s_TargetAcquiredTick = 0;
            return;
        }

        // 6. Se o usuário já estiver pressionando o botão de tiro manualmente, não interfere
        bool isUserShooting = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (isUserShooting)
        {
            s_TargetAcquiredTick = 0;
            return;
        }

        // 7. Arma válida em mãos (IDs 22 a 34 - Pistolas, Shotguns, SMGs, Rifles, Snipers)
        uint32_t weaponId = GTA::GetCurrentWeaponId();
        if (weaponId < 22 || weaponId > 34)
        {
            s_TargetAcquiredTick = 0;
            return;
        }

        // 8. Checagem de munição e estado interno da arma
        __try
        {
            void* pLocalPed = *reinterpret_cast<void**>(0x00B7CD98);
            if (pLocalPed)
            {
                uint8_t slot = *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(pLocalPed) + 0x718);
                uintptr_t weaponPtr = reinterpret_cast<uintptr_t>(pLocalPed) + 0x5A0 + slot * 0x1C;
                uint32_t ammoInClip = *reinterpret_cast<uint32_t*>(weaponPtr + 0x8);
                uint32_t weaponState = *reinterpret_cast<uint32_t*>(weaponPtr + 0x10);

                if (ammoInClip == 0 || weaponState == 3)
                {
                    s_TargetAcquiredTick = 0;
                    return;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        // 9. Detecção de alvo sob a retícula
        int targetId = -1;
        bool hasTarget = IsTargetUnderCrosshair(targetId);
        now = GetTickCount64();

        if (!hasTarget)
        {
            s_TargetAcquiredTick = 0;
            return;
        }

        // 10. Alvo detectado: gerencia temporizador de reação
        if (s_TargetAcquiredTick == 0)
        {
            s_TargetAcquiredTick = now;
        }

        int reactionDelay = g_MenuState.triggerBot.reactionDelay;
        if (reactionDelay < 0) reactionDelay = 0;
        if (reactionDelay > 200) reactionDelay = 200;

        if ((now - s_TargetAcquiredTick) >= static_cast<ULONGLONG>(reactionDelay))
        {
            // Cooldown de segurança por cadência de arma (evita disparo travado)
            ULONGLONG minCooldown = 90; // Automáticas (M4, AK, Tec9, MP5, Uzi)
            if (weaponId == 24) minCooldown = 280;                         // Desert Eagle (permite c-bug rápido)
            else if (weaponId == 25 || weaponId == 27) minCooldown = 380; // Shotguns
            else if (weaponId == 26) minCooldown = 180;                    // Sawnoff
            else if (weaponId == 33 || weaponId == 34) minCooldown = 550; // Sniper / Country
            else if (weaponId == 22 || weaponId == 23) minCooldown = 150; // 9mm Pistols

            if ((now - s_LastFireTick) >= minCooldown && !s_NeedReleaseLMB)
            {
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                s_ClickPressTick = now;
                s_NeedReleaseLMB = true;
                s_LastFireTick = now;
                s_TargetAcquiredTick = now;
            }
        }
    }
}
