#pragma once
#include <windows.h>
#include "../../Config/Config.h"
#include "TargetSelector.h"
#include "../../Render/ImGui/imgui.h"

struct AimAssistState
{
    bool isActive = false;
    int targetId = -1;
    float deltaX = 0.0f;
    float deltaY = 0.0f;
    float smoothDeltaX = 0.0f;
    float smoothDeltaY = 0.0f;
    float accumulatedX = 0.0f;
    float accumulatedY = 0.0f;
    float smoothFactor = 1.0f;
    ImVec2 targetScreen = ImVec2(0, 0);
    ImVec2 screenCenter = ImVec2(0, 0);
    int outputX = 0;
    int outputY = 0;
    bool applied = false;
};

struct SilentAimDiagnostic
{
    bool active = false;
    int targetId = -1;
    char targetName[32] = "";
    char boneName[16] = "";
    float targetWorldPos[3] = { 0.0f, 0.0f, 0.0f };
    float predictedImpact[3] = { 0.0f, 0.0f, 0.0f };
    float screenDist = 0.0f;
    int hitChance = 100;
    bool hitChancePassed = false;
    uint64_t lastShotTick = 0;
};

namespace AimAssist
{
    // Verifica se a condição de ativação (Always, While Aiming, While Shooting, Aim+Shoot) está satisfeita
    bool CheckActivationCondition(int activationMode);

    // Retorna se o AimAssist está atualmente ativo (Aimbot enabled + Target válido + Ativação satisfeita)
    bool IsActive();

    // Retorna o estado atual do cálculo de assistência
    const AimAssistState& GetState();

    // Aplica fisicamente o movimento relativo de mira ao jogo via mouse_event
    bool Apply(int moveX, int moveY);

    // Processa o cálculo suave sobre o TargetInfo recebido do TargetSelector
    void Process(const TargetInfo& target, const WeaponAimConfig& config, ImVec2 screenCenter);

    // Renderiza a telemetria visual de diagnóstico (Vetor Center -> Target, Vetor de Smooth e HUD de variáveis)
    void RenderDebugVisuals(ImVec2 screenCenter, const WeaponAimConfig& config);

    // Reseta o estado e acumuladores (ex.: quando alvo é perdido ou troca de alvo)
    void Reset();

    // Diagnóstico e telemetria de Silent Aim
    const SilentAimDiagnostic& GetSilentDiagnostic();
    void ResetSilentDiagnostic();
    uint64_t GetLastLocalShotTick();
}

