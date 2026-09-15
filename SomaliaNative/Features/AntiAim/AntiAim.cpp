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
    static bool  s_DesyncActive = false;
    static bool  s_JitterState = false;
    static bool  s_WasInvertebredActive = false;
    static float s_NetworkQuat[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    void Initialize()
    {
        s_SpinAngle = 0.0f;
        s_ChokedTicks = 0;
        s_RealAngle = 0.0f;
        s_FakeAngle = 0.0f;
        s_IsActive = false;
        s_DesyncActive = false;
        s_WasInvertebredActive = false;
        s_NetworkQuat[0] = 0.0f;
        s_NetworkQuat[1] = 0.0f;
        s_NetworkQuat[2] = 0.0f;
        s_NetworkQuat[3] = 1.0f;
    }

    void Reset()
    {
        s_IsActive = false;
        s_DesyncActive = false;
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
                    if (pMatrix)
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

    bool IsDesyncActive()
    {
        return s_DesyncActive || g_MenuState.antiAim.desync;
    }

    bool IsInvertebredActive()
    {
        return g_MenuState.antiAim.invertebred;
    }

    void GetNetworkQuaternion(float outQuat[4])
    {
        if (!outQuat) return;
        outQuat[0] = s_NetworkQuat[0];
        outQuat[1] = s_NetworkQuat[1];
        outQuat[2] = s_NetworkQuat[2];
        outQuat[3] = s_NetworkQuat[3];
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
        // O Invertebred agora atua exclusivamente na sincronizacao de rede (TwistPlayer do Blume),
        // portanto o GTA SA local permanece 100% normal e ereto na tela do jogador.
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

            // Garante que o modelo local permaneca normal
            ProcessInvertebred(pedAddr);

            if (!g_MenuState.antiAim.enabled && !g_MenuState.antiAim.fakeLag && !g_MenuState.antiAim.desync && !g_MenuState.antiAim.invertebred && !g_MenuState.player.antiHS)
            {
                s_IsActive = false;
                s_DesyncActive = false;
                s_ChokedTicks = 0;
                return;
            }

            // Se o menu estiver aberto ou o chat ativo no SAMP, suspende Anti-Aim
            if (g_MenuState.menuOpen || SAMP::HasActiveCursor())
            {
                s_IsActive = false;
                s_DesyncActive = false;
                s_ChokedTicks = 0;
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

            // 2. Leitura do angulo atual (Real Angle)
            // IMPORTANTE: O heading local pedAddr + 0x558 NUNCA é modificado pelo Desync Angle.
            // O jogador local mira, anda e atira normalmente na sua própria tela sem desorientação!
            float* pHeading = reinterpret_cast<float*>(pedAddr + 0x558);
            if (!pHeading || IsBadReadPtr(pHeading, sizeof(float)))
                return;

            float realHeading = *pHeading;
            const float PI = 3.14159265f;
            const float TWO_PI = 6.283185307f;

            s_RealAngle = realHeading * (180.0f / PI);

            float fakeYaw = realHeading;
            bool doDesyncSpin = g_MenuState.antiAim.desync || (g_MenuState.antiAim.enabled && g_MenuState.antiAim.yawMode == 2);

            // 3. Desync Angle: o giro contínuo é calculado e transmitido exclusivamente para os outros jogadores
            // ATENÇÃO: NÃO para ao mirar! Continua girando normalmente na rede mesmo mirando ou atirando!
            if (doDesyncSpin)
            {
                s_DesyncActive = true;
                int speed = g_MenuState.antiAim.spinSpeed;
                if (speed < 1) speed = 15;
                if (speed > 50) speed = 50;

                s_SpinAngle += (static_cast<float>(speed) * 0.04f);
                while (s_SpinAngle > TWO_PI) s_SpinAngle -= TWO_PI;
                while (s_SpinAngle < 0.0f) s_SpinAngle += TWO_PI;

                fakeYaw += s_SpinAngle;
            }
            else
            {
                s_DesyncActive = false;
            }

            // Checagem de mira/tiro apenas para Anti-Aim tradicional (Pitch de combate)
            bool isAiming = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
            bool isShooting = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            bool pauseCombatAA = (isAiming || isShooting);

            // 4. Modos adicionais de Yaw (Anti-Aim tradicional)
            if (g_MenuState.antiAim.enabled && !pauseCombatAA)
            {
                switch (g_MenuState.antiAim.yawMode)
                {
                case 1: // Backward (180 graus)
                    fakeYaw += PI;
                    break;

                case 2: // Spinbot (já processado em doDesyncSpin)
                    break;

                case 3: // Jitter
                    s_JitterState = !s_JitterState;
                    fakeYaw += (s_JitterState ? 1.4f : -1.4f);
                    break;

                case 4: // Random
                    fakeYaw += static_cast<float>(rand() % 628) / 100.0f;
                    break;

                default:
                    break;
                }
            }

            // Normaliza fakeYaw no intervalo [0, 2*PI]
            while (fakeYaw > TWO_PI) fakeYaw -= TWO_PI;
            while (fakeYaw < 0.0f) fakeYaw += TWO_PI;

            s_FakeAngle = fakeYaw * (180.0f / PI);

            // 5. Modos de Pitch (apenas afeta quaternion de rede)
            float pitchAngle = 0.0f;
            if (g_MenuState.antiAim.enabled && !pauseCombatAA)
            {
                switch (g_MenuState.antiAim.pitchMode)
                {
                case 1: // Emotion / Down (-89 graus)
                    pitchAngle = -1.55334f;
                    break;
                case 2: // Up (89 graus)
                    pitchAngle = 1.55334f;
                    break;
                case 3: // Zero (0 graus)
                    pitchAngle = 0.0f;
                    break;
                default:
                    break;
                }
            }
            else if (g_MenuState.player.antiHS)
            {
                // Micro-inclinação de combate para frente (-25.7 graus = -0.45 rad):
                // Desloca o osso da cabeça na tela dos adversários para baixo/frente,
                // fazendo tiros na cabeça visual passarem no vácuo ("varar o tiro")
                pitchAngle = -0.45f;
            }

            // 6. Cálculo do Quaternion normalizado para a rede (SA-MP)
            if (g_MenuState.antiAim.invertebred)
            {
                s_WasInvertebredActive = true;
                // Invertebred oficial (Blume TwistPlayer): uniform random unit quaternion (Shoemake)
                // Gera quaternions 100% unitários e normalizados, aceitos por qualquer anti-cheat
                float u1 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                float u2 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                float u3 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                float sqrt1MinusU1 = sqrtf(1.0f - u1);
                float sqrtU1 = sqrtf(u1);
                s_NetworkQuat[0] = sqrt1MinusU1 * sinf(TWO_PI * u2); // X
                s_NetworkQuat[1] = sqrt1MinusU1 * cosf(TWO_PI * u2); // Y
                s_NetworkQuat[2] = sqrtU1 * sinf(TWO_PI * u3);        // Z
                s_NetworkQuat[3] = sqrtU1 * cosf(TWO_PI * u3);        // W
            }
            else
            {
                if (s_WasInvertebredActive)
                {
                    ProcessInvertebred(pedAddr);
                }

                float halfPitch = pitchAngle * 0.5f;
                float halfYaw   = fakeYaw * 0.5f;

                // Formato de Quaternion DirectX / SA-MP: [0] = X, [1] = Y, [2] = Z, [3] = W
                s_NetworkQuat[0] = sinf(halfPitch) * cosf(halfYaw);
                s_NetworkQuat[1] = -sinf(halfPitch) * sinf(halfYaw);
                s_NetworkQuat[2] = sinf(halfYaw) * cosf(halfPitch);
                s_NetworkQuat[3] = cosf(halfPitch) * cosf(halfYaw);
            }

            // 7. Reflete na memória stOnFootData do jogador local no SA-MP
            if (SAMP::IsLoaded())
            {
                uintptr_t pOnFoot = SAMP::GetLocalPlayerOnFootData();
                if (pOnFoot && !IsBadWritePtr(reinterpret_cast<void*>(pOnFoot), 68))
                {
                    float* pQuat = reinterpret_cast<float*>(pOnFoot + 18);
                    pQuat[0] = s_NetworkQuat[0];
                    pQuat[1] = s_NetworkQuat[1];
                    pQuat[2] = s_NetworkQuat[2];
                    pQuat[3] = s_NetworkQuat[3];

                    if (g_MenuState.antiAim.invertebred)
                    {
                        static const uint16_t s_TwistAnimIDs[5] = { 972, 973, 974, 975, 977 };
                        *reinterpret_cast<uint16_t*>(pOnFoot + 64) = s_TwistAnimIDs[rand() % 5];
                        *reinterpret_cast<uint16_t*>(pOnFoot + 66) = 12082;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void MutateOnFootPacket(unsigned char* data, int length)
    {
        if (!data || length < 35) return;
        if (data[0] != 207) return;

        // Se menu aberto ou cursor ativo, não muta
        if (g_MenuState.menuOpen || SAMP::HasActiveCursor()) return;

        static uint64_t s_lastMutateLog = 0;
        uint64_t nowTick = GetTickCount64();
        if (nowTick - s_lastMutateLog >= 3000)
        {
            Logger::Log("[ANTIAIM][NET] MutateOnFootPacket ativo! invertebred=%d desync=%d aa=%d fakeYaw=%.1f",
                g_MenuState.antiAim.invertebred ? 1 : 0,
                g_MenuState.antiAim.desync ? 1 : 0,
                g_MenuState.antiAim.enabled ? 1 : 0,
                s_FakeAngle);
            s_lastMutateLog = nowTick;
        }

        // 1. Invertebred: estilo TwistPlayer oficial do Blume (UGBASE)
        // Funciona continuamente na rede, mesmo ao atirar ou mirar!
        if (g_MenuState.antiAim.invertebred)
        {
            // Quaternions aleatórios uniformes unitários distorcem a malha esquelética nos outros clientes
            float* pQuat = reinterpret_cast<float*>(data + 19);
            float u1 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            float u2 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            float u3 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            const float TWO_PI = 6.283185307f;
            float sqrt1MinusU1 = sqrtf(1.0f - u1);
            float sqrtU1 = sqrtf(u1);
            pQuat[0] = sqrt1MinusU1 * sinf(TWO_PI * u2); // X
            pQuat[1] = sqrt1MinusU1 * cosf(TWO_PI * u2); // Y
            pQuat[2] = sqrtU1 * sinf(TWO_PI * u3);        // Z
            pQuat[3] = sqrtU1 * cosf(TWO_PI * u3);        // W

            if (length >= 68)
            {
                static const uint16_t s_TwistAnimIDs[5] = { 972, 973, 974, 975, 977 };
                *reinterpret_cast<uint16_t*>(data + 65) = s_TwistAnimIDs[rand() % 5];
                *reinterpret_cast<uint16_t*>(data + 67) = 12082;
            }
            return;
        }

        // 2. Desync Angle: gira o boneco na visão dos outros jogadores
        // Continua operando MESMO ao mirar (RMB) e atirar (LMB), sem parar!
        if (g_MenuState.antiAim.desync || (g_MenuState.antiAim.enabled && s_IsActive))
        {
            float* pQuat = reinterpret_cast<float*>(data + 19);
            pQuat[0] = s_NetworkQuat[0];
            pQuat[1] = s_NetworkQuat[1];
            pQuat[2] = s_NetworkQuat[2];
            pQuat[3] = s_NetworkQuat[3];
        }
        else if (g_MenuState.player.antiHS)
        {
            // 3. Anti-HS Hitbox Shift (Ghost Head / Cabeça Fantasma):
            // Aplica a micro-inclinação de combate para frente (-25.7 graus) sincronizada com s_NetworkQuat.
            // No motor de colisão do adversário, a hitbox da cabeça (Bone 9) se desloca para baixo.
            // Tiros mirando na cabeça visual passam no ar ("varam o tiro")!
            float* pQuat = reinterpret_cast<float*>(data + 19);
            pQuat[0] = s_NetworkQuat[0];
            pQuat[1] = s_NetworkQuat[1];
            pQuat[2] = s_NetworkQuat[2];
            pQuat[3] = s_NetworkQuat[3];
        }
    }

    void ProcessDamageBitStream(unsigned char* data, int bitCount)
    {
        if (!data || bitCount < 16) return;
        if (!g_MenuState.player.antiHS) return;

        // Se for um pacote ID_RPC (32), só processa se for RPC 115 (RPC_GiveTakeDamage)
        if (data[0] == 32 && data[1] != 115) return;

        __try
        {
            // SA-MP RPC 115 (RPC_Damage):
            // 1. bool bGiveOrTake (1 bit: true=TakeDamage, false=GiveDamage)
            // 2. uint16_t issuerId (16 bits)
            // 3. float fDamage (32 bits)
            // 4. uint32_t weaponId (32 bits)
            // 5. uint32_t bodyPart (32 bits)
            // Bone 9 = HEAD, Bone 3 = CHEST

            // Varredura por bits (para dados compactados com bit-packing padrão do RakNet):
            for (int bitOffset = 64; bitOffset <= (bitCount - 32); ++bitOffset)
            {
                uint32_t boneVal = 0;
                for (int b = 0; b < 32; ++b)
                {
                    int curBit = bitOffset + b;
                    if ((data[curBit >> 3] & (0x80 >> (curBit & 7))) != 0)
                    {
                        boneVal |= (1u << b);
                    }
                }

                if (boneVal == 9) // Bone 9 (HEAD)
                {
                    // Reescreve osso 9 -> 3 (CHEST)
                    for (int b = 0; b < 32; ++b)
                    {
                        int curBit = bitOffset + b;
                        uint32_t bit3 = (3u >> b) & 1u;
                        if (bit3)
                            data[curBit >> 3] |= (0x80 >> (curBit & 7));
                        else
                            data[curBit >> 3] &= ~(0x80 >> (curBit & 7));
                    }

                    // Lê e ajusta o float de dano (64 bits antes de bodyPart)
                    int dmgBitOffset = bitOffset - 64;
                    if (dmgBitOffset >= 0)
                    {
                        uint32_t rawDmg = 0;
                        for (int b = 0; b < 32; ++b)
                        {
                            int curBit = dmgBitOffset + b;
                            if ((data[curBit >> 3] & (0x80 >> (curBit & 7))) != 0)
                                rawDmg |= (1u << b);
                        }

                        float dmg = *reinterpret_cast<float*>(&rawDmg);
                        if (dmg > g_MenuState.player.antiHSDamageCap && dmg < 500.0f)
                        {
                            dmg = g_MenuState.player.antiHSDamageCap;
                            uint32_t newRawDmg = *reinterpret_cast<uint32_t*>(&dmg);
                            for (int b = 0; b < 32; ++b)
                            {
                                int curBit = dmgBitOffset + b;
                                uint32_t bitVal = (newRawDmg >> b) & 1u;
                                if (bitVal)
                                    data[curBit >> 3] |= (0x80 >> (curBit & 7));
                                else
                                    data[curBit >> 3] &= ~(0x80 >> (curBit & 7));
                            }
                        }
                        Logger::Log("[SOMALIA][ANTI-HS] Interceptado RPC 115 bitOffset=%d! Osso 9 (Cabeca) -> 3 (Peito). Dano regulado para %.1f HP",
                            bitOffset, dmg);
                    }
                    return;
                }
            }

            // Varredura por bytes alinhados (fallback para dados byte-aligned)
            int byteCount = (bitCount + 7) / 8;
            for (int i = 8; i <= byteCount - 4; ++i)
            {
                uint32_t* pBone = reinterpret_cast<uint32_t*>(data + i);
                if (*pBone == 9)
                {
                    *pBone = 3;
                    if (i >= 8)
                    {
                        float* pDmg = reinterpret_cast<float*>(data + i - 8);
                        if (!IsBadReadPtr(pDmg, sizeof(float)) && *pDmg > g_MenuState.player.antiHSDamageCap && *pDmg < 500.0f)
                        {
                            *pDmg = g_MenuState.player.antiHSDamageCap;
                        }
                    }
                    Logger::Log("[SOMALIA][ANTI-HS] Interceptado RPC 115 byte-aligned! Osso 9 -> 3.");
                    return;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void ProcessDamageRPC(unsigned char* data, int length, int bitCount)
    {
        ProcessDamageBitStream(data, bitCount > 0 ? bitCount : (length * 8));
    }
}
