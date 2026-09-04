#include "AntiAim.h"
#include "../../Config/Config.h"
#include "../../Core/Logger.h"
#include "../../Engine/GTA/GTA.h"
#include "../../Engine/SAMP/SAMP.h"
#include "../../Core/RuntimeState.h"
#include <math.h>
#include <stdlib.h>

namespace AntiAim
{
    static float s_SpinAngle = 0.0f;
    static int   s_ChokedTicks = 0;
    static float s_RealAngle = 0.0f;
    static float s_FakeAngle = 0.0f;
    static bool  s_IsActive = false;
    static bool  s_JitterState = false;
    static bool  s_WasInvertebredActive = false;
    static float s_InvertebredAngle = 0.0f;

    void Initialize()
    {
        s_SpinAngle = 0.0f;
        s_ChokedTicks = 0;
        s_RealAngle = 0.0f;
        s_FakeAngle = 0.0f;
        s_IsActive = false;
        s_WasInvertebredActive = false;
        s_InvertebredAngle = 0.0f;
    }

    void Reset()
    {
        s_IsActive = false;
        s_ChokedTicks = 0;

        if (s_WasInvertebredActive)
        {
            __try
            {
                void* pLocalPed = *reinterpret_cast<void**>(0x00B7CD98);
                if (pLocalPed)
                {
                    uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pLocalPed);
                    uintptr_t pMatrix = *reinterpret_cast<uintptr_t*>(pedAddr + 0x14);
                    if (pMatrix && !IsBadWritePtr(reinterpret_cast<void*>(pMatrix), 0x40))
                    {
                        *reinterpret_cast<float*>(pMatrix + 0x20) = 0.0f;
                        *reinterpret_cast<float*>(pMatrix + 0x24) = 0.0f;
                        *reinterpret_cast<float*>(pMatrix + 0x28) = 1.0f;
                    }

                    uintptr_t pClump = *reinterpret_cast<uintptr_t*>(pedAddr + 0x18);
                    if (pClump && !IsBadReadPtr(reinterpret_cast<void*>(pClump), 8))
                    {
                        uintptr_t pFrame = *reinterpret_cast<uintptr_t*>(pClump + 4);
                        if (pFrame && !IsBadWritePtr(reinterpret_cast<void*>(pFrame + 0x10), 0x30))
                        {
                            float* pUp = reinterpret_cast<float*>(pFrame + 0x20);
                            pUp[0] = 0.0f;
                            pUp[1] = 0.0f;
                            pUp[2] = 1.0f;
                        }
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
            s_WasInvertebredActive = false;
        }
    }

    int GetChokedTicks()
    {
        return s_ChokedTicks;
    }

    float GetRealAngle()
    {
        return s_RealAngle;
    }

    float GetFakeAngle()
    {
        return s_FakeAngle;
    }

    bool IsActive()
    {
        return s_IsActive;
    }

    bool IsInvertebredActive()
    {
        return g_MenuState.antiAim.invertebred;
    }

    bool ShouldChokeSyncPacket()
    {
        if (!g_MenuState.antiAim.fakeLag || !s_IsActive)
        {
            s_ChokedTicks = 0;
            return false;
        }

        int limit = g_MenuState.antiAim.fakeLagLimit;
        if (limit < 1) limit = 1;
        if (limit > 16) limit = 16;

        s_ChokedTicks++;
        if (s_ChokedTicks <= limit)
        {
            // Retorna true para instruir o hook de rede a suprimir este pacote de sincronização
            return true;
        }

        s_ChokedTicks = 0;
        return false;
    }

    static void ProcessInvertebred(uintptr_t pedAddr)
    {
        if (!g_MenuState.antiAim.invertebred)
        {
            if (s_WasInvertebredActive)
            {
                uintptr_t pMatrix = *reinterpret_cast<uintptr_t*>(pedAddr + 0x14);
                if (pMatrix && !IsBadWritePtr(reinterpret_cast<void*>(pMatrix), 0x40))
                {
                    *reinterpret_cast<float*>(pMatrix + 0x20) = 0.0f;
                    *reinterpret_cast<float*>(pMatrix + 0x24) = 0.0f;
                    *reinterpret_cast<float*>(pMatrix + 0x28) = 1.0f;
                }

                uintptr_t pClump = *reinterpret_cast<uintptr_t*>(pedAddr + 0x18);
                if (pClump && !IsBadReadPtr(reinterpret_cast<void*>(pClump), 8))
                {
                    uintptr_t pFrame = *reinterpret_cast<uintptr_t*>(pClump + 4);
                    if (pFrame && !IsBadWritePtr(reinterpret_cast<void*>(pFrame + 0x10), 0x30))
                    {
                        float* pUp = reinterpret_cast<float*>(pFrame + 0x20);
                        pUp[0] = 0.0f;
                        pUp[1] = 0.0f;
                        pUp[2] = 1.0f;
                    }
                }

                s_WasInvertebredActive = false;
            }
            return;
        }

        // 1. Modifica pacote de sincronização em rede do SA-MP (stOnFootData)
        if (SAMP::IsLoaded())
        {
            uintptr_t pOnFootData = SAMP::GetLocalPlayerOnFootData();
            if (pOnFootData && !IsBadWritePtr(reinterpret_cast<void*>(pOnFootData), 68))
            {
                // Randomiza os 4 eixos do quaternion fQuaternion[0..3] (0.0f a 256.0f)
                float* pQuat = reinterpret_cast<float*>(pOnFootData + 18);
                pQuat[0] = static_cast<float>(rand() % 256);
                pQuat[1] = static_cast<float>(rand() % 256);
                pQuat[2] = static_cast<float>(rand() % 256);
                pQuat[3] = static_cast<float>(rand() % 256);

                // Animação e flags de desync
                static const uint16_t s_animIds[] = { 0x0B03, 0x0477, 0x045D, 0x0443, 0x0429 };
                *reinterpret_cast<uint16_t*>(pOnFootData + 64) = s_animIds[rand() % 5];
                *reinterpret_cast<uint16_t*>(pOnFootData + 66) = 12082; // Anim flags originais do script Blume
            }
        }

        // 2. Torção, rotação e inversão do modelo 3D no GTA SA local
        s_InvertebredAngle += 0.25f;
        if (s_InvertebredAngle > 6.283185f) s_InvertebredAngle -= 6.283185f;

        // Jitter e rotação no heading local para visualização imediata em 3ª pessoa
        float* pHeading = reinterpret_cast<float*>(pedAddr + 0x558);
        if (pHeading && !IsBadWritePtr(pHeading, sizeof(float)))
        {
            *pHeading += (sinf(s_InvertebredAngle) * 0.45f);
        }

        // Torção na matriz de RenderWare Clump (Frame Up vector)
        uintptr_t pClump = *reinterpret_cast<uintptr_t*>(pedAddr + 0x18);
        if (pClump && !IsBadReadPtr(reinterpret_cast<void*>(pClump), 8))
        {
            uintptr_t pFrame = *reinterpret_cast<uintptr_t*>(pClump + 4);
            if (pFrame && !IsBadWritePtr(reinterpret_cast<void*>(pFrame + 0x10), 0x30))
            {
                float* pUp = reinterpret_cast<float*>(pFrame + 0x20);
                pUp[0] = sinf(s_InvertebredAngle) * 0.65f;
                pUp[1] = cosf(s_InvertebredAngle) * 0.65f;
                pUp[2] = -0.95f; // Inverte o corpo de cabeça para baixo
            }
        }

        // Torção no Placeable Matrix
        uintptr_t pMatrix = *reinterpret_cast<uintptr_t*>(pedAddr + 0x14);
        if (pMatrix && !IsBadWritePtr(reinterpret_cast<void*>(pMatrix), 0x40))
        {
            float* pUpX = reinterpret_cast<float*>(pMatrix + 0x20);
            float* pUpY = reinterpret_cast<float*>(pMatrix + 0x24);
            float* pUpZ = reinterpret_cast<float*>(pMatrix + 0x28);

            *pUpX = sinf(s_InvertebredAngle) * 0.70f;
            *pUpY = cosf(s_InvertebredAngle) * 0.70f;
            *pUpZ = -0.98f;

            s_WasInvertebredActive = true;
        }
    }

    void Update()
    {
        if (!RuntimeState::IsPlayerAlive())
        {
            Reset();
            return;
        }

        __try
        {
            void* pLocalPed = RuntimeState::GetLocalPed();
            if (!pLocalPed)
            {
                Reset();
                return;
            }

            uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pLocalPed);

            // Garante que o hook de rede do RakClient esteja ativo no SA-MP
            if (SAMP::IsLoaded())
            {
                SAMP::EnsureRakHook();
            }

            // Processa Invertebred
            ProcessInvertebred(pedAddr);

            if (!g_MenuState.antiAim.enabled && !g_MenuState.antiAim.fakeLag)
            {
                s_IsActive = false;
                s_ChokedTicks = 0;
                return;
            }

            // Se o menu estiver aberto ou o chat ativo no SAMP, suspende Anti-Aim
            if (g_MenuState.menuOpen || SAMP::HasActiveCursor())
            {
                s_IsActive = false;
                s_ChokedTicks = 0;
                return;
            }

            // Durante a mira manual (RMB) ou disparo (LMB), suspende para permitir precisao total
            bool isAiming = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
            bool isShooting = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            if (isAiming || isShooting)
            {
                s_IsActive = false;
                return;
            }

            float health = *reinterpret_cast<float*>(pedAddr + 0x540);
            if (health <= 0.0f)
            {
                Reset();
                return;
            }

            void* pVeh = *reinterpret_cast<void**>(pedAddr + 0x58C);
            if (pVeh)
            {
                Reset();
                return;
            }

            s_IsActive = true;

            // 1. Processamento de Fake Lag (Choked Ticks)
            if (g_MenuState.antiAim.fakeLag)
            {
                int limit = g_MenuState.antiAim.fakeLagLimit;
                if (limit < 1) limit = 1;
                if (limit > 16) limit = 16;

                s_ChokedTicks++;
                if (s_ChokedTicks > limit)
                {
                    s_ChokedTicks = 0;
                }
            }
            else
            {
                s_ChokedTicks = 0;
            }

            if (!g_MenuState.antiAim.enabled)
                return;

            // 2. Leitura do angulo atual (Real Angle)
            float* pHeading = reinterpret_cast<float*>(pedAddr + 0x558);
            s_RealAngle = *pHeading * (180.0f / 3.14159265f);

            float newHeading = *pHeading;
            const float PI = 3.14159265f;

            // 3. Modos de Yaw
            switch (g_MenuState.antiAim.yawMode)
            {
            case 1: // Backward (180 graus)
                newHeading += PI;
                break;

            case 2: // Spinbot
                s_SpinAngle += (static_cast<float>(g_MenuState.antiAim.spinSpeed) * 0.04f);
                if (s_SpinAngle > 2.0f * PI) s_SpinAngle -= 2.0f * PI;
                newHeading += s_SpinAngle;
                break;

            case 3: // Jitter
                s_JitterState = !s_JitterState;
                newHeading += (s_JitterState ? 1.4f : -1.4f);
                break;

            case 4: // Random
                newHeading += static_cast<float>(rand() % 628) / 100.0f;
                break;

            default:
                break;
            }

            // 4. Desync
            if (g_MenuState.antiAim.desync)
            {
                newHeading += 0.65f; // Desloca ~37 graus
            }

            // Normaliza no intervalo [0, 2*PI]
            while (newHeading > 2.0f * PI) newHeading -= 2.0f * PI;
            while (newHeading < 0.0f) newHeading += 2.0f * PI;

            // Aplica no Ped
            *pHeading = newHeading;
            s_FakeAngle = newHeading * (180.0f / PI);

            // 5. Modos de Pitch
            float pitchAngle = 0.0f;
            bool applyPitch = false;
            switch (g_MenuState.antiAim.pitchMode)
            {
            case 1: // Emotion / Down (-89 graus)
                pitchAngle = -1.55334f;
                applyPitch = true;
                break;
            case 2: // Up (89 graus)
                pitchAngle = 1.55334f;
                applyPitch = true;
                break;
            case 3: // Zero (0 graus)
                pitchAngle = 0.0f;
                applyPitch = true;
                break;
            default:
                break;
            }

            if (applyPitch)
            {
                // Aplica na variável de inclinação de mira nativa do GTA SA CPed (+0x5BC)
                float* pAimZ = reinterpret_cast<float*>(pedAddr + 0x5BC);
                if (pAimZ && !IsBadWritePtr(pAimZ, sizeof(float)))
                {
                    *pAimZ = pitchAngle;
                }

                // Se SA-MP estiver ativo e Invertebred não estiver sobrepondo, reflete no quaternion da rede
                if (SAMP::IsLoaded() && !g_MenuState.antiAim.invertebred)
                {
                    uintptr_t pOnFoot = SAMP::GetLocalPlayerOnFootData();
                    if (pOnFoot && !IsBadWritePtr(reinterpret_cast<void*>(pOnFoot), 68))
                    {
                        float* pQuat = reinterpret_cast<float*>(pOnFoot + 18);
                        float halfPitch = pitchAngle * 0.5f;
                        float halfYaw = newHeading * 0.5f;
                        pQuat[0] = sinf(halfPitch) * cosf(halfYaw);
                        pQuat[1] = -sinf(halfPitch) * sinf(halfYaw);
                        pQuat[2] = cosf(halfPitch) * sinf(halfYaw);
                        pQuat[3] = cosf(halfPitch) * cosf(halfYaw);
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
}
