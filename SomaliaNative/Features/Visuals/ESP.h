#pragma once
#include <windows.h>
#include <d3d9.h>
#include "../../Render/ImGui/imgui.h"

namespace ESP
{
    // Converte coordenadas do mundo 3D (X, Y, Z) para coordenadas de tela 2D (ImVec2)
    bool WorldToScreen(float worldX, float worldY, float worldZ, ImVec2& outScreen);

    // Loop de renderizacao principal do ESP, FOV Circle, Entidades, Hitmarker e Damage Informer
    void Render();

    // Dispara indicador de hitmarker na tela
    void TriggerHitmarker();

    // Adiciona evento numerico ao Damage Informer
    void AddDamageInformer(float worldX, float worldY, float worldZ, float damage);

    // Limpa buffers visuais e historico de dano
    void Reset();
}
