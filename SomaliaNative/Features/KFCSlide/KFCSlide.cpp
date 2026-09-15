#include "KFCSlide.h"
#include "../../Config/Config.h"
#include "../../Core/RuntimeState.h"
#include "../../Core/Logger.h"
#include "../../Engine/SAMP/SAMP.h"
#include <math.h>

namespace KFCSlide
{
    static bool       s_KFCActive = false;        // Ativado ao mirar, desativa 2s apos soltar
    static ULONGLONG  s_LastAimReleaseTick = 0;   // Momento em que soltou a mira
    static bool       s_WasAiming = false;
    static bool       s_IsSliding = false;
    static bool       s_WasSliding = false;
    static ULONGLONG  s_SlideStartTick = 0;
    static float      s_CurrentSpeed = 0.0f;

    // Janela de 0.8 segundos apos soltar a mira para desativar o KFC (800 ms)
    static const ULONGLONG KFC_DEACTIVATE_DELAY_MS = 800;

    // Lista completa original das 64 animacoes do KFC Slide 2.0 (Blast.hk)
    static const char* s_KFCAnimations[] = {
        "WALK_PLAYER",
        "GUNCROUCHFWD",
        "GUNCROUCHBWD",
        "GUNMOVE_BWD",
        "GUNMOVE_FWD",
        "GUNMOVE_L",
        "GUNMOVE_R",
        "RUN_GANG1",
        "JOG_FEMALEA",
        "JOG_MALEA",
        "RUN_CIVI",
        "RUN_CSAW",
        "RUN_FAT",
        "RUN_FATOLD",
        "RUN_OLD",
        "RUN_ROCKET",
        "RUN_WUZI",
        "SPRINT_WUZI",
        "WALK_ARMED",
        "WALK_CIVI",
        "WALK_CSAW",
        "WALK_DRUNK",
        "WALK_FAT",
        "WALK_FATOLD",
        "WALK_GANG1",
        "WALK_GANG2",
        "WALK_OLD",
        "WALK_SHUFFLE",
        "WALK_START",
        "WALK_START_ARMED",
        "WALK_START_CSAW",
        "WALK_START_ROCKET",
        "WALK_WUZI",
        "WOMAN_WALKBUSY",
        "WOMAN_WALKFATOLD",
        "WOMAN_WALKNORM",
        "WOMAN_WALKOLD",
        "WOMAN_RUNFATOLD",
        "WOMAN_WALKPRO",
        "WOMAN_WALKSEXY",
        "WOMAN_WALKSHOP",
        "RUN_1ARMED",
        "RUN_ARMED",
        "RUN_PLAYER",
        "WALK_ROCKET",
        "CLIMB_IDLE",
        "MUSCLESPRINT",
        "CLIMB_PULL",
        "CLIMB_STAND",
        "CLIMB_STAND_FINISH",
        "SWIM_BREAST",
        "SWIM_CRAWL",
        "SWIM_DIVE_UNDER",
        "SWIM_GLIDE",
        "MUSCLERUN",
        "WOMAN_RUN",
        "WOMAN_RUNBUSY",
        "WOMAN_RUNPANIC",
        "WOMAN_RUNSEXY",
        "SPRINT_CIVI",
        "SPRINT_PANIC",
        "SWAT_RUN",
        "FATSPRINT"
    };

    static const size_t NUM_ANIMS = sizeof(s_KFCAnimations) / sizeof(s_KFCAnimations[0]);

    // Altera a velocidade de reproducao da animacao nativa do GTA SA 1.0 US (equivalente ao setCharAnimSpeed)
    static void SetCharAnimSpeed(void* pLocalPed, const char* animName, float speed)
    {
        if (!pLocalPed) return;
        __try
        {
            uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pLocalPed);
            void* pClump = *reinterpret_cast<void**>(pedAddr + 0x18);
            if (!pClump) return;

            typedef void* (__cdecl* tRpAnimBlendClumpGetAssociation)(void* clump, const char* name);
            static tRpAnimBlendClumpGetAssociation fnGetAssoc = reinterpret_cast<tRpAnimBlendClumpGetAssociation>(0x004D6870);

            void* pAssoc = fnGetAssoc(pClump, animName);
            if (pAssoc)
            {
                *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(pAssoc) + 0x24) = speed;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    static void ResetAnimSpeeds(void* pLocalPed)
    {
        if (!pLocalPed) return;
        for (const char* animName : s_KFCAnimations)
        {
            SetCharAnimSpeed(pLocalPed, animName, 1.0f);
        }
    }

    void Initialize()
    {
        Reset();
        Logger::Log("[SOMALIA][KFC] Modulo KFCSlide inicializado (Forma original: ativo ao mirar, desativa 2s apos soltar).");
    }

    void Reset()
    {
        __try
        {
            void* pLocalPed = RuntimeState::GetLocalPed();
            if (pLocalPed)
            {
                ResetAnimSpeeds(pLocalPed);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }

        s_KFCActive = false;
        s_LastAimReleaseTick = 0;
        s_WasAiming = false;
        s_IsSliding = false;
        s_WasSliding = false;
        s_SlideStartTick = 0;
        s_CurrentSpeed = 0.0f;
    }

    bool IsActive()
    {
        return s_IsSliding;
    }

    float GetCurrentSpeed()
    {
        return s_CurrentSpeed;
    }

    void Update()
    {
        // 1. Verificações de prontidão e estado
        if (!g_MenuState.kfcSlide.enabled || !RuntimeState::IsPlayerAlive())
        {
            if (s_KFCActive || s_IsSliding || s_WasAiming) Reset();
            return;
        }

        // Se o menu da Somalia ou chat/diálogo do SA-MP estiver aberto, suspende
        if (g_MenuState.menuOpen || SAMP::HasActiveCursor())
        {
            if (s_KFCActive || s_IsSliding || s_WasAiming) Reset();
            return;
        }

        __try
        {
            void* pLocalPed = RuntimeState::GetLocalPed();
            if (!pLocalPed)
            {
                if (s_KFCActive || s_IsSliding || s_WasAiming) Reset();
                return;
            }

            uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pLocalPed);

            // Verifica se está em veículo
            if (*reinterpret_cast<void**>(pedAddr + 0x58C) != nullptr)
            {
                if (s_KFCActive || s_IsSliding || s_WasAiming) Reset();
                return;
            }

            // Verifica vida
            float health = *reinterpret_cast<float*>(pedAddr + 0x540);
            if (health <= 0.0f)
            {
                if (s_KFCActive || s_IsSliding || s_WasAiming) Reset();
                return;
            }

            // setPlayerNeverGetsTired (equivalente ao script original da Blast.hk)
            *reinterpret_cast<float*>(0x00B7CEE4) = 100.0f;

            ULONGLONG currentTick = GetTickCount64();
            bool isAiming = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

            // 2. ATIVAÇÃO: Só ativa quando o jogador aperta o botão direito (mira)
            if (isAiming)
            {
                s_KFCActive = true;
            }

            // Se o KFC não foi ativado por mira, fica totalmente desligado em repouso
            if (!s_KFCActive)
            {
                return;
            }

            // 3. Máquina de gatilho idêntica à forma original que estava funcionando:
            // "if isKeyDown(VK_RBUTTON) and not slot3 then slot3 = true"
            if (isAiming && !s_WasAiming)
            {
                s_WasAiming = true;
            }
            // "elseif not isKeyDown(VK_RBUTTON) and slot3 then slot3 = false; slot4 = true; slot5 = os.clock() * 1000"
            else if (!isAiming && s_WasAiming)
            {
                s_WasAiming = false;
                s_IsSliding = true;
                s_SlideStartTick = currentTick;
                s_LastAimReleaseTick = currentTick; // Registra momento em que soltou a mira
            }

            // Janela do slide da forma que estava (100 ms padrão)
            int duration = 100;

            // "if slot4 and slot8 <= os.clock() * 1000 - slot5 then slot4 = false"
            if (s_IsSliding && (currentTick - s_SlideStartTick >= static_cast<ULONGLONG>(duration)))
            {
                s_IsSliding = false;
            }

            // 4. DESATIVAÇÃO APÓS 1.3 SEGUNDOS:
            // "ao eu soltar o botao direito, 1.3 segundos depois desativa o kfc, e so ativa novamente quando eu apertar o botao direito"
            if (!isAiming && s_LastAimReleaseTick > 0)
            {
                if (currentTick - s_LastAimReleaseTick >= KFC_DEACTIVATE_DELAY_MS)
                {
                    // Passaram 1.3 segundos desde que soltou a mira: desativa o KFC completamente!
                    Reset();
                    return;
                }
            }

            // 5. Aplicação do multiplicador de velocidade da forma que estava
            float speedMult = g_MenuState.kfcSlide.speed;
            if (speedMult < 1.0f) speedMult = 1.0f;
            if (speedMult > 15.0f) speedMult = 15.0f;

            // Enquanto s_IsSliding (100ms) está ativo, aplica speedMult; nos 2 segundos seguintes, aplica 1.0f
            float currentAnimSpeed = s_IsSliding ? speedMult : 1.0f;

            for (const char* animName : s_KFCAnimations)
            {
                SetCharAnimSpeed(pLocalPed, animName, currentAnimSpeed);
            }

            // 6. Impulso físico direcionado idêntico à forma que estava durante o slide
            if (s_IsSliding)
            {
                bool keyW = (GetAsyncKeyState('W') & 0x8000) != 0;
                bool keyS = (GetAsyncKeyState('S') & 0x8000) != 0;
                bool keyA = (GetAsyncKeyState('A') & 0x8000) != 0;
                bool keyD = (GetAsyncKeyState('D') & 0x8000) != 0;

                float* pMoveX = reinterpret_cast<float*>(pedAddr + 0x44);
                float* pMoveY = reinterpret_cast<float*>(pedAddr + 0x48);
                float curVx = *pMoveX;
                float curVy = *pMoveY;
                float curSpeed = sqrtf(curVx * curVx + curVy * curVy);

                void* pMatrix = *reinterpret_cast<void**>(pedAddr + 0x14);
                float fwdX = 0.0f, fwdY = 0.0f;
                float rgtX = 0.0f, rgtY = 0.0f;

                if (pMatrix)
                {
                    float* mat = reinterpret_cast<float*>(pMatrix);
                    rgtX = mat[0]; rgtY = mat[1];
                    fwdX = mat[4]; fwdY = mat[5];
                }
                else
                {
                    float heading = *reinterpret_cast<float*>(pedAddr + 0x558);
                    fwdX = -sinf(heading); fwdY = cosf(heading);
                    rgtX = cosf(heading);  rgtY = sinf(heading);
                }

                float dirX = 0.0f, dirY = 0.0f;
                if (keyW) { dirX += fwdX; dirY += fwdY; }
                if (keyS) { dirX -= fwdX; dirY -= fwdY; }
                if (keyD) { dirX += rgtX; dirY += rgtY; }
                if (keyA) { dirX -= rgtX; dirY -= rgtY; }

                float dirLen = sqrtf(dirX * dirX + dirY * dirY);
                if (dirLen > 0.001f)
                {
                    dirX /= dirLen;
                    dirY /= dirLen;
                }
                else if (curSpeed > 0.02f)
                {
                    dirX = curVx / curSpeed;
                    dirY = curVy / curSpeed;
                    dirLen = 1.0f;
                }

                if (dirLen > 0.001f)
                {
                    float targetPhysSpeed = 0.080f * speedMult;
                    *pMoveX = dirX * targetPhysSpeed;
                    *pMoveY = dirY * targetPhysSpeed;
                }

                s_CurrentSpeed = currentAnimSpeed;
            }
            else
            {
                s_CurrentSpeed = 0.0f;
            }

            s_WasSliding = s_IsSliding;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            Reset();
        }
    }
}
