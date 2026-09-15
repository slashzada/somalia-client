#include "AimAssist.h"
#include "Aimbot.h"
#include "RageBot.h"
#include "../../Core/Logger.h"
#include "../../Core/RuntimeState.h"
#include <math.h>
#include <stdio.h>

namespace AimAssist
{
    static AimAssistState s_State = {};
    static uint64_t s_LastLogTick = 0;
    static ULONGLONG s_DoubleTapTick = 0;
    static bool s_DoubleTapFired = false;
    static uint64_t s_LastLocalShotTick = 0;

    bool CheckActivationCondition(int activationMode)
    {
        if (g_MenuState.menuOpen)
            return false;

        switch (activationMode)
        {
        case 0: // Always
            return true;

        case 1: // While Aiming (RMB)
            return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

        case 2: // While Shooting (LMB)
            return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        case 3: // While Aiming + Shooting
            return ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0) &&
                   ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);

        default:
            return false;
        }
    }

    bool IsActive()
    {
        return s_State.isActive;
    }

    const AimAssistState& GetState()
    {
        return s_State;
    }

    void Reset()
    {
        s_State.isActive = false;
        s_State.targetId = -1;
        s_State.deltaX = 0.0f;
        s_State.deltaY = 0.0f;
        s_State.smoothDeltaX = 0.0f;
        s_State.smoothDeltaY = 0.0f;
        s_State.accumulatedX = 0.0f;
        s_State.accumulatedY = 0.0f;
        s_State.smoothFactor = 1.0f;
        s_State.outputX = 0;
        s_State.outputY = 0;
        s_State.applied = false;
        s_DoubleTapFired = false;
    }

    bool Apply(int moveX, int moveY)
    {
        if (moveX == 0 && moveY == 0)
            return false;

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

    void Process(const TargetInfo& target, const WeaponAimConfig& config, ImVec2 screenCenter)
    {
        if (!RuntimeState::IsPlayerAlive())
        {
            Reset();
            return;
        }

        if (!g_MenuState.legitBot.enabled || !config.enabled || !target.valid || target.ped == nullptr)
        {
            Reset();
            return;
        }

        if (!CheckActivationCondition(config.activationMode))
        {
            s_State.isActive = false;
            s_State.outputX = 0;
            s_State.outputY = 0;
            s_State.accumulatedX = 0.0f;
            s_State.accumulatedY = 0.0f;
            s_State.applied = false;
            s_DoubleTapFired = false;
            return;
        }

        if (target.playerId != s_State.targetId)
        {
            s_State.accumulatedX = 0.0f;
            s_State.accumulatedY = 0.0f;
            s_State.targetId = target.playerId;
        }

        float deltaX = target.screenPosition.x - screenCenter.x;
        float deltaY = target.screenPosition.y - screenCenter.y;

        // Exploit: Lag Peek
        if (g_MenuState.legitBot.exploitLagPeek && target.ped)
        {
            float* pTargetVelX = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(target.ped) + 0x44);
            float* pTargetVelY = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(target.ped) + 0x48);
            if (pTargetVelX && pTargetVelY)
            {
                deltaX += (*pTargetVelX) * 20.0f;
                deltaY += (*pTargetVelY) * 20.0f;
            }
        }

        if (isnan(deltaX) || isinf(deltaX) || isnan(deltaY) || isinf(deltaY))
        {
            Reset();
            return;
        }

        float smooth = config.smooth;
        if (smooth < 1.0f) smooth = 1.0f;
        if (isnan(smooth) || isinf(smooth)) smooth = 6.0f;

        float sensScale = 0.18f;
        float smoothDeltaX = (deltaX * sensScale) / smooth;
        float smoothDeltaY = (deltaY * sensScale) / smooth;

        float maxStep = 15.0f;
        if (smoothDeltaX > maxStep) smoothDeltaX = maxStep;
        else if (smoothDeltaX < -maxStep) smoothDeltaX = -maxStep;

        if (smoothDeltaY > maxStep) smoothDeltaY = maxStep;
        else if (smoothDeltaY < -maxStep) smoothDeltaY = -maxStep;

        bool isShootingNow = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        // Exploit: Hide Shots
        if (g_MenuState.legitBot.exploitHideShots && isShootingNow)
        {
            void* pLocalPed = *reinterpret_cast<void**>(0x00B7CD98);
            if (pLocalPed)
            {
                uint8_t slot = *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(pLocalPed) + 0x718);
                uintptr_t weaponPtr = reinterpret_cast<uintptr_t>(pLocalPed) + 0x5A0 + slot * 0x1C;
                *reinterpret_cast<uint32_t*>(weaponPtr + 0x10) = 1; // WEAPON_STATE_READY
            }
        }

        // Exploit: Double Tap
        if (g_MenuState.legitBot.exploitDoubleTap)
        {
            uint64_t now = GetTickCount64();
            if (isShootingNow && !s_DoubleTapFired)
            {
                s_DoubleTapTick = now;
                s_DoubleTapFired = true;
            }
            else if (s_DoubleTapFired && (now - s_DoubleTapTick > 60) && (now - s_DoubleTapTick < 130))
            {
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                s_DoubleTapFired = false;
            }
            else if (now - s_DoubleTapTick >= 130)
            {
                s_DoubleTapFired = false;
            }
        }

        if (isShootingNow || s_DoubleTapFired)
        {
            s_LastLocalShotTick = GetTickCount64();
        }

        // Se o RageBot estiver ativamente habilitado e mirando neste quadro, cede a atuacao fisica de mouse
        // para prevenir conflitos de deltas no SendInput
        if (g_MenuState.rageBot.enabled && RageBot::GetState().isActive)
        {
            s_State.accumulatedX = 0.0f;
            s_State.accumulatedY = 0.0f;
            s_State.outputX = 0;
            s_State.outputY = 0;
            s_State.applied = false;
            return;
        }

        s_State.accumulatedX += smoothDeltaX;
        s_State.accumulatedY += smoothDeltaY;

        int moveX = static_cast<int>(s_State.accumulatedX);
        int moveY = static_cast<int>(s_State.accumulatedY);

        s_State.outputX = moveX;
        s_State.outputY = moveY;
        s_State.applied = false;

        if (moveX != 0 || moveY != 0)
        {
            s_State.accumulatedX -= static_cast<float>(moveX);
            s_State.accumulatedY -= static_cast<float>(moveY);

            s_State.applied = Apply(moveX, moveY);
        }

        s_State.isActive = true;
        s_State.targetScreen = target.screenPosition;
        s_State.screenCenter = screenCenter;
        s_State.deltaX = deltaX;
        s_State.deltaY = deltaY;
        s_State.smoothDeltaX = smoothDeltaX;
        s_State.smoothDeltaY = smoothDeltaY;
        s_State.smoothFactor = smooth;

        uint64_t currentTick = GetTickCount64();
        if (currentTick - s_LastLogTick >= 1000)
        {
            Logger::Log("[SOMALIA][AIMASSIST] active=1 target=%d delta=(%.1f,%.1f) smooth=(%.2f,%.2f) output=(%d,%d) applied=%s",
                s_State.targetId, s_State.deltaX, s_State.deltaY,
                s_State.smoothDeltaX, s_State.smoothDeltaY,
                s_State.outputX, s_State.outputY,
                s_State.applied ? "SIM" : "NAO");

            int activeGroup = Aimbot::GetActiveWeaponGroup();
            bool keyState = CheckActivationCondition(config.activationMode);
            Logger::Log("[SOMALIA][AIMASSIST] weapon=%u profile=%s smooth=%.1f | enabled=%d activationMode=%d keyState=%d targetValid=%d active=%d",
                Aimbot::GetCurrentWeaponId(), Aimbot::GetWeaponProfileName(activeGroup),
                config.smooth, config.enabled ? 1 : 0, config.activationMode,
                keyState ? 1 : 0, target.valid ? 1 : 0, s_State.isActive ? 1 : 0);

            s_LastLogTick = currentTick;
        }
    }

    void RenderDebugVisuals(ImVec2 screenCenter, const WeaponAimConfig& config)
    {
        if (!s_State.isActive || !config.drawSmoothVector)
            return;

        ImDrawList* draw = ImGui::GetForegroundDrawList();
        if (!draw) return;

        ImVec2 targetPos(screenCenter.x + s_State.deltaX, screenCenter.y + s_State.deltaY);
        draw->AddLine(screenCenter, targetPos, IM_COL32(255, 180, 40, 160), 1.0f);

        ImVec2 stepEnd(screenCenter.x + s_State.smoothDeltaX * 3.0f, screenCenter.y + s_State.smoothDeltaY * 3.0f);
        draw->AddLine(screenCenter, stepEnd, IM_COL32(50, 255, 50, 255), 2.5f);
        draw->AddCircleFilled(stepEnd, 3.5f, IM_COL32(50, 255, 50, 255));

        char dbg1[96], dbg2[96], dbg3[96];
        snprintf(dbg1, sizeof(dbg1), "TARGET: (%.0f, %.0f) | CENTER: (%.0f, %.0f)",
            s_State.targetScreen.x, s_State.targetScreen.y, s_State.screenCenter.x, s_State.screenCenter.y);
        snprintf(dbg2, sizeof(dbg2), "DELTA: (%.1f, %.1f) | SMOOTH: %.1f -> STEP: (%.1f, %.1f)",
            s_State.deltaX, s_State.deltaY, s_State.smoothFactor, s_State.smoothDeltaX, s_State.smoothDeltaY);
        snprintf(dbg3, sizeof(dbg3), "FINAL OUTPUT: (%d, %d) | APPLIED: %s",
            s_State.outputX, s_State.outputY, s_State.applied ? "SIM" : "NAO");

        draw->AddText(ImVec2(screenCenter.x + 12, screenCenter.y + 14), IM_COL32(0, 0, 0, 255), dbg1);
        draw->AddText(ImVec2(screenCenter.x + 11, screenCenter.y + 13), IM_COL32(255, 255, 255, 240), dbg1);

        draw->AddText(ImVec2(screenCenter.x + 12, screenCenter.y + 28), IM_COL32(0, 0, 0, 255), dbg2);
        draw->AddText(ImVec2(screenCenter.x + 11, screenCenter.y + 27), IM_COL32(50, 255, 50, 240), dbg2);

        draw->AddText(ImVec2(screenCenter.x + 12, screenCenter.y + 42), IM_COL32(0, 0, 0, 255), dbg3);
        draw->AddText(ImVec2(screenCenter.x + 11, screenCenter.y + 41), IM_COL32(255, 200, 50, 240), dbg3);
    }
}
