#pragma once
#include <windows.h>
#include <stdint.h>
#include "../../Config/Config.h"
#include "TargetSelector.h"
#include "../../Render/ImGui/imgui.h"

// ─────────────────────────────────────────────────────────────
// ESTADO INTERNO DEDICADO DO RAGEBOT (Isolamento Total)
// ─────────────────────────────────────────────────────────────
struct RageBotState
{
    bool isActive = false;
    bool enabledPass = false;
    bool activationPass = false;
    bool fovPass = false;
    int targetId = -1;
    char boneName[32] = "HEAD";
    char statusString[64] = "STANDBY";
    float deltaX = 0.0f;
    float deltaY = 0.0f;
    float stepX = 0.0f;
    float stepY = 0.0f;
    float aggressiveness = 100.0f;
    ImVec2 targetScreen = ImVec2(0, 0);
    ImVec2 screenCenter = ImVec2(0, 0);
};

namespace RageBot
{
    // Converte a porcentagem de FOV configurada (0-100%) para o raio exato em pixels na tela
    float GetFovRadius(float fovPercent);

    // Identifica o grupo da arma atualmente empunhada no jogo (0: Snipers, 1: Pistols, 2: Rifles, 3: Shotguns)
    int GetActiveWeaponGroup();

    // Retorna a referencia do perfil Rage da arma ativa ou selecionada na UI
    RageWeaponConfig& GetCurrentWeaponProfile();

    // Retorna o alvo atualmente selecionado pelo Ragebot
    const TargetInfo& GetCurrentTarget();

    // Retorna o estado interno atual de calculo e diagnostico do Ragebot
    const RageBotState& GetState();

    // Retorna o nome amigavel do perfil de arma
    const char* GetWeaponProfileName(int group);

    // Checagem de ativacao independente (Always, Aim, Shoot, Aim+Shoot)
    bool CheckActivationCondition(int activationMode);

    // Reseta o estado interno do Ragebot
    void Reset();

    // Atualizacao de selecao de alvos e calculo do vetor agressivo
    void Update();

    // Renderizacao dos indicadores visuais, FOV proprio e vetor de convergencia agressivo
    void Render();
}
