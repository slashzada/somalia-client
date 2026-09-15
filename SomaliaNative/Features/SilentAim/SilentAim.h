#pragma once
#include <windows.h>
#include <stdint.h>
#include "../../Config/Config.h"
#include "../Aimbot/TargetSelector.h"
#include "../../Render/ImGui/imgui.h"

// ─────────────────────────────────────────────────────────────
// ESTADO E DIAGNÓSTICO DO SILENT AIM DEDICADO
// ─────────────────────────────────────────────────────────────
struct SilentAimState
{
    bool isActive = false;
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

struct SilentShotTarget
{
    bool valid = false;
    int playerId = -1;
    float boneWorldPos[3] = { 0.0f, 0.0f, 0.0f };
    float pedCenterPos[3] = { 0.0f, 0.0f, 0.0f };
    int boneId = 8;
    char boneName[16] = "HEAD";
    char playerName[32] = "";
};

namespace SilentAim
{
    // Converte a porcentagem de FOV configurada (0-100%) para o raio exato em pixels na tela
    float GetFovRadius(float fovPercent);

    // Identifica o grupo da arma atualmente empunhada no GTA SA
    int GetActiveWeaponGroup();

    // Retorna o perfil Silent da arma ativa ou selecionada na UI
    SilentWeaponConfig& GetCurrentWeaponProfile();

    // Retorna o alvo atualmente rastreado pelo SilentAim
    const TargetInfo& GetCurrentTarget();

    // Retorna o estado atual de telemetria do SilentAim
    const SilentAimState& GetState();

    // Reseta estado interno e telemetria
    void Reset();

    // Limpa o alvo atual (morte, respawn, shutdown)
    void ClearTarget();

    // Verifica se a condição de ativação está satisfeita (Always, Aim, Shoot, Aim+Shoot)
    bool CheckActivationCondition(int activationMode);

    // Registra e obtém o timestamp do último disparo local
    void RecordShotTick();
    uint64_t GetLastShotTick();

    // Resolução de osso (incluindo bone == 4 RANDOM)
    int ResolveBoneIndex(int boneOption);
    const char* GetBoneName(int boneId);

    // Atualização de seleção de alvos em tempo de tela (para indicadores visuais e FOV)
    void Update();

    // Renderização visual independente: FOV Circle, Target Marker [ O ], Tracers
    void Render();

    // Validação e seleção FRESH executada exatamente no momento do envio do pacote de tiro
    SilentShotTarget FindTargetOnShot();

    // Inicialização do hook de motor GTA e handlers
    void Initialize();
    void Shutdown();

    // Callback pré-disparo invocado diretamente pelo hook CWeapon::FireInstantHit (0x00742300)
    // Redireciona o vetor da bala sem jamais puxar a mira ou movimentar a tela do jogador
    bool OnNativeWeaponFire(void* pWeapon, void* pPed, void* pOrigin, void* pTarget,
                            float outSavedFront[3], float outSavedTarget[3], bool& outModifiedTarget);

    // Callback para interceptação do traçante visual do GTA SA (CBulletTraces::AddTrace 0x00723C10)
    // Redireciona o início do traçante visual para o céu no disparo de Sniper HS Celestial
    bool OnBulletTracePre(float* pOrigin, float* pTarget, float& fThickness, uint32_t& lifeTime, uint8_t& visibility);

    // Mutação de pacotes RakNet ID_BULLET_SYNC (206)
    void MutateBulletSyncPacket(unsigned char* data, int length);
}
