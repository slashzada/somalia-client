#pragma once
#include <windows.h>
#include "../../Config/Config.h"
#include "TargetSelector.h"

namespace Aimbot
{
    // Converte a porcentagem de FOV configurada (0-100%) para o raio exato em pixels na tela
    float GetFovRadius(float fovPercent);

    // Identifica o grupo da arma atualmente empunhada pelo jogador (0: Snipers, 1: Pistols, 2: Rifles, 3: Shotguns)
    int GetActiveWeaponGroup();

    // Retorna a referência do perfil da arma ativa ou selecionada
    WeaponAimConfig& GetCurrentWeaponProfile();

    // Retorna o alvo atualmente selecionado pelo TargetSelector
    const TargetInfo& GetCurrentTarget();

    // Retorna o ID numérico nativo da arma empunhada no GTA SA
    uint32_t GetCurrentWeaponId();

    // Retorna o nome amigável do perfil de arma
    const char* GetWeaponProfileName(int group);

    // Atualização de seleção de alvos e diagnósticos (sem mouse_event)
    void Update();

    // Limpa o alvo atual (ex: na morte ou desligamento)
    void ClearTarget();

    // Renderização do Target Indicator e sincronização do FOV Circle
    void Render();
}
