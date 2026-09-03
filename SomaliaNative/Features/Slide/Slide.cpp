#include "Slide.h"
#include "../../Config/Config.h"
#include "../../Engine/SAMP/SAMP.h"
#include "../../Core/Logger.h"
#include "../../Core/RuntimeState.h"
#include <math.h>

namespace Slide
{
    enum class SlideState
    {
        IDLE = 0,
        WAITING_MARGIN,
        WAITING_C,
        WAITING_SWITCH_DELAY,
        WAITING_Q_RELEASE
    };

    static SlideState s_State = SlideState::IDLE;
    static ULONGLONG  s_TargetTick = 0;
    static ULONGLONG  s_LastShotTick = 0;
    static bool       s_WasAiming = false;
    static bool       s_IsKeyDownC = false;
    static bool       s_IsKeyDownQ = false;
    static uint8_t    s_OriginalSlot = 0;

    void Reset()
    {
        if (s_IsKeyDownC)
        {
            keybd_event('C', 0, KEYEVENTF_KEYUP, 0);
            s_IsKeyDownC = false;
        }
        if (s_IsKeyDownQ)
        {
            keybd_event('Q', 0, KEYEVENTF_KEYUP, 0);
            s_IsKeyDownQ = false;
        }
        s_State = SlideState::IDLE;
        s_WasAiming = false;
        s_OriginalSlot = 0;
    }

    void Update()
    {
        if (!RuntimeState::IsPlayerAlive())
        {
            if (s_State != SlideState::IDLE) Reset();
            return;
        }

        if (!g_MenuState.slide.enabled)
        {
            if (s_State != SlideState::IDLE) Reset();
            return;
        }

        // Se o menu Somalia ou o chat/diálogo do SA-MP estiverem abertos, não executa
        if (g_MenuState.menuOpen || SAMP::HasActiveCursor())
        {
            if (s_State != SlideState::IDLE) Reset();
            return;
        }

        ULONGLONG currentTick = GetTickCount64();

        __try
        {
            void* pLocalPed = *reinterpret_cast<void**>(0x00B7CD98);
            if (!pLocalPed)
            {
                if (s_State != SlideState::IDLE) Reset();
                return;
            }

            uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pLocalPed);

            // Verifica se está vivo
            float health = *reinterpret_cast<float*>(pedAddr + 0x540);
            if (health <= 0.0f)
            {
                if (s_State != SlideState::IDLE) Reset();
                return;
            }

            // Verifica se está em veículo
            void* pVeh = *reinterpret_cast<void**>(pedAddr + 0x58C);
            if (pVeh)
            {
                if (s_State != SlideState::IDLE) Reset();
                return;
            }

            // Monitora disparos com botão esquerdo ou tiro ativo do ped
            bool isShooting = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            if (isShooting)
            {
                s_LastShotTick = currentTick;
            }

            // Monitora estado da mira (botão direito)
            bool isAimingNow = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

            // Transição: ESTAVA MIRANDO -> SOLTOU A MIRA
            if (s_WasAiming && !isAimingNow && s_State == SlideState::IDLE)
            {
                // Verifica movimento do jogador (teclas W, A, S, D ou velocidade física)
                bool isMoving = (GetAsyncKeyState('W') & 0x8000) ||
                                (GetAsyncKeyState('A') & 0x8000) ||
                                (GetAsyncKeyState('S') & 0x8000) ||
                                (GetAsyncKeyState('D') & 0x8000);

                if (!isMoving)
                {
                    float vx = *reinterpret_cast<float*>(pedAddr + 0x44);
                    float vy = *reinterpret_cast<float*>(pedAddr + 0x48);
                    if (sqrtf(vx * vx + vy * vy) > 0.05f)
                    {
                        isMoving = true;
                    }
                }

                if (isMoving)
                {
                    // Obtém slot e ID da arma ativa atual no ped
                    uint8_t curSlot = *reinterpret_cast<uint8_t*>(pedAddr + 0x718);
                    s_OriginalSlot = curSlot;
                    uint32_t weaponId = *reinterpret_cast<uint32_t*>(pedAddr + 0x5A0 + curSlot * 0x1C);

                    int margin = 0;
                    if (weaponId == 24)                          margin = g_MenuState.slide.marginDeagle;
                    else if (weaponId >= 25 && weaponId <= 27)   margin = g_MenuState.slide.marginShotgun;
                    else if (weaponId == 33 || weaponId == 34)   margin = g_MenuState.slide.marginSniper;
                    else if (weaponId == 31)                     margin = g_MenuState.slide.marginM4;
                    else if (weaponId == 30)                     margin = g_MenuState.slide.marginAK47;

                    int elapsed = static_cast<int>(currentTick - s_LastShotTick);
                    int delayWait = (margin > elapsed) ? (margin - elapsed) : 0;

                    s_TargetTick = currentTick + delayWait;
                    s_State = SlideState::WAITING_MARGIN;
                }
            }

            s_WasAiming = isAimingNow;

            // Máquina de estados assíncrona não-bloqueante
            switch (s_State)
            {
            case SlideState::WAITING_MARGIN:
                if (currentTick >= s_TargetTick)
                {
                    if (g_MenuState.slide.cSlideActive)
                    {
                        keybd_event('C', 0, 0, 0);
                        s_IsKeyDownC = true;
                        int durC = g_MenuState.slide.durationC >= 5 ? g_MenuState.slide.durationC : 5;
                        s_TargetTick = currentTick + durC;
                        s_State = SlideState::WAITING_C;

                        // Aplica impulso de velocidade de deslocamento no início do slide
                        float vx = *reinterpret_cast<float*>(pedAddr + 0x44);
                        float vy = *reinterpret_cast<float*>(pedAddr + 0x48);
                        float curSpeed = sqrtf(vx * vx + vy * vy);
                        if (curSpeed > 0.01f)
                        {
                            float boost = g_MenuState.slide.slideBoost > 0.5f ? g_MenuState.slide.slideBoost : 1.8f;
                            float targetSpeed = 0.28f * boost;
                            *reinterpret_cast<float*>(pedAddr + 0x44) = (vx / curSpeed) * targetSpeed;
                            *reinterpret_cast<float*>(pedAddr + 0x48) = (vy / curSpeed) * targetSpeed;
                        }
                    }
                    else if (g_MenuState.slide.autoSlideActive)
                    {
                        s_TargetTick = currentTick + g_MenuState.slide.delayTroca;
                        s_State = SlideState::WAITING_SWITCH_DELAY;
                    }
                    else
                    {
                        s_State = SlideState::IDLE;
                    }
                }
                break;

            case SlideState::WAITING_C:
                // Mantém a velocidade durante todo o tempo em que a tecla C está agachando
                {
                    float vx = *reinterpret_cast<float*>(pedAddr + 0x44);
                    float vy = *reinterpret_cast<float*>(pedAddr + 0x48);
                    float curSpeed = sqrtf(vx * vx + vy * vy);
                    if (curSpeed > 0.01f)
                    {
                        float boost = g_MenuState.slide.slideBoost > 0.5f ? g_MenuState.slide.slideBoost : 1.8f;
                        float targetSpeed = 0.26f * boost;
                        *reinterpret_cast<float*>(pedAddr + 0x44) = (vx / curSpeed) * targetSpeed;
                        *reinterpret_cast<float*>(pedAddr + 0x48) = (vy / curSpeed) * targetSpeed;
                    }
                }

                if (currentTick >= s_TargetTick)
                {
                    if (s_IsKeyDownC)
                    {
                        keybd_event('C', 0, KEYEVENTF_KEYUP, 0);
                        s_IsKeyDownC = false;
                    }

                    if (g_MenuState.slide.autoSlideActive)
                    {
                        s_TargetTick = currentTick + g_MenuState.slide.delayTroca;
                        s_State = SlideState::WAITING_SWITCH_DELAY;
                    }
                    else
                    {
                        s_State = SlideState::IDLE;
                    }
                }
                break;

            case SlideState::WAITING_SWITCH_DELAY:
                if (currentTick >= s_TargetTick)
                {
                    // Troca rápida para o soco / slot 0 para cancelar o recuo
                    uint8_t curSlot = *reinterpret_cast<uint8_t*>(pedAddr + 0x718);
                    if (curSlot > 0)
                    {
                        *reinterpret_cast<uint8_t*>(pedAddr + 0x718) = 0;
                    }

                    keybd_event('Q', 0, 0, 0);
                    s_IsKeyDownQ = true;
                    s_TargetTick = currentTick + 20;
                    s_State = SlideState::WAITING_Q_RELEASE;
                }
                break;

            case SlideState::WAITING_Q_RELEASE:
                if (currentTick >= s_TargetTick)
                {
                    if (s_IsKeyDownQ)
                    {
                        keybd_event('Q', 0, KEYEVENTF_KEYUP, 0);
                        s_IsKeyDownQ = false;
                    }

                    // Restaura slot de arma original para que o jogador possa atirar novamente
                    if (s_OriginalSlot > 0)
                    {
                        *reinterpret_cast<uint8_t*>(pedAddr + 0x718) = s_OriginalSlot;
                    }

                    s_State = SlideState::IDLE;
                }
                break;

            default:
                break;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            Reset();
        }
    }
}
