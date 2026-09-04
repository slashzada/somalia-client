#include "SAMP.h"
#include <atomic>
#include "../../Core/Main.h"
#include "../GTA/GTA.h"
#include "../../Core/Logger.h"
#include "../../Config/Config.h"
#include "../../Features/Aimbot/Aimbot.h"
#include "../../Features/Aimbot/AimAssist.h"
#include "../../Features/Aimbot/TargetSelector.h"
#include "../../Features/AntiAim/AntiAim.h"
#include "../../Core/RuntimeState.h"
#include "../../../Common/SafeMemory.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace SAMP
{
    struct VersionConfig
    {
        const char* name;
        uintptr_t infoOffset;
        uintptr_t miscInfoOffset;
        uintptr_t toggleCursorOffset;
        uintptr_t unlockCamOffset;
        uintptr_t poolsOffset;
        uintptr_t playerPoolOffset;
        uintptr_t remotePlayerOffset;
        uintptr_t isListedOffset;
        uintptr_t localPlayerIdOffset;
    };

    static Version s_Version = Version::Unknown;
    static VersionConfig s_Config = {};
    static bool s_Initialized = false;

    bool IsLoaded()
    {
        return (GetModuleHandleA("samp.dll") != NULL);
    }

    uintptr_t GetBaseAddress()
    {
        return reinterpret_cast<uintptr_t>(GetModuleHandleA("samp.dll"));
    }

    static void DetectVersion()
    {
        if (s_Initialized)
            return;

        uintptr_t sampBase = GetBaseAddress();
        if (!sampBase)
            return;

        __try
        {
            PIMAGE_DOS_HEADER pDos = reinterpret_cast<PIMAGE_DOS_HEADER>(sampBase);
            if (!pDos || !SafeMemory::IsValidReadPtr(pDos, sizeof(IMAGE_DOS_HEADER)) || pDos->e_magic != IMAGE_DOS_SIGNATURE)
                return;

            PIMAGE_NT_HEADERS pNt = reinterpret_cast<PIMAGE_NT_HEADERS>(sampBase + pDos->e_lfanew);
            if (!pNt || !SafeMemory::IsValidReadPtr(pNt, sizeof(IMAGE_NT_HEADERS)) || pNt->Signature != IMAGE_NT_SIGNATURE)
                return;

            DWORD entryPoint = pNt->OptionalHeader.AddressOfEntryPoint;
            DWORD timeDateStamp = pNt->FileHeader.TimeDateStamp;

            // Verificação determinística por campos oficiais do PE Header
            // 0.3.7-R1: EP 0x31DF13, TimeDateStamp 0x554D0DE8
            if (entryPoint == 0x31DF13 || timeDateStamp == 0x554D0DE8)
            {
                s_Version = Version::R1;
                s_Config = { "0.3.7-R1", 0x21A0F8, 0x21A10C, 0x9BD30, 0x9BC10, 0x3CD, 0x18, 0x2E, 0xFDE, 0x4 };
            }
            // 0.3.7-R2: EP 0x3195DD, TimeDateStamp 0x559D040A
            else if (entryPoint == 0x3195DD || timeDateStamp == 0x559D040A)
            {
                s_Version = Version::R2;
                s_Config = { "0.3.7-R2", 0x21A0F8, 0x21A10C, 0x9BD30, 0x9BC10, 0x3CD, 0x18, 0x2E, 0xFDE, 0x4 };
            }
            // 0.3.7-R3: EP 0xCC4D0, TimeDateStamp 0x5C0B4243 / 0x5C0B3E7A
            else if (entryPoint == 0xCC4D0 || timeDateStamp == 0x5C0B4243 || timeDateStamp == 0x5C0B3E7A)
            {
                s_Version = Version::R3;
                s_Config = { "0.3.7-R3", 0x26E8DC, 0x26E8F4, 0x9FFE0, 0x9FEC0, 0x3DE, 0x8, 0x4, 0xFB4, 0x2F1C };
            }
            // 0.3.7-R4: EP 0xCBCB0, TimeDateStamp 0x5DE3DCB8
            else if (entryPoint == 0xCBCB0 || timeDateStamp == 0x5DE3DCB8)
            {
                s_Version = Version::R4;
                s_Config = { "0.3.7-R4", 0x26EA04, 0x26EA0C, 0xA0750, 0xA0630, 0x3DE, 0x8, 0x4, 0xFB4, 0x2F1C };
            }
            // 0.3.7-R5: EP 0xCBC90, TimeDateStamp 0x6009E839
            else if (entryPoint == 0xCBC90 || timeDateStamp == 0x6009E839)
            {
                s_Version = Version::R5;
                s_Config = { "0.3.7-R5", 0x26EB94, 0x26EBAC, 0xA0890, 0xA0770, 0x3DE, 0x4, 0x4, 0xFB4, 0x2F1C };
            }
            // 0.3.DL-1: EP 0xCB180, TimeDateStamp 0x5A707993
            else if (entryPoint == 0xCB180 || timeDateStamp == 0x5A707993)
            {
                s_Version = Version::DL;
                s_Config = { "0.3.DL-1", 0x2ACA14, 0x2ACA24, 0xA0530, 0xA0410, 0x3DE, 0x8, 0x4, 0xFB4, 0x2F1C };
            }
            else
            {
                // Verificação secundária por byte pattern da função ToggleCursor com validação estrita (5 bytes)
                unsigned char* pR1 = reinterpret_cast<unsigned char*>(sampBase + 0x9BD30);
                unsigned char* pR3 = reinterpret_cast<unsigned char*>(sampBase + 0x9FFE0);
                unsigned char* pR4 = reinterpret_cast<unsigned char*>(sampBase + 0xA0750);
                unsigned char* pR5 = reinterpret_cast<unsigned char*>(sampBase + 0xA0890);
                unsigned char* pDL = reinterpret_cast<unsigned char*>(sampBase + 0xA0530);

                if (pR1 && SafeMemory::IsValidReadPtr(pR1, 5) && pR1[0] == 0x55 && pR1[1] == 0x8B && pR1[2] == 0xEC && pR1[3] == 0x83 && pR1[4] == 0xEC)
                {
                    s_Version = Version::R1;
                    s_Config = { "0.3.7-R1 (Signature)", 0x21A0F8, 0x21A10C, 0x9BD30, 0x9BC10, 0x3CD, 0x18, 0x2E, 0xFDE, 0x4 };
                }
                else if (pR3 && SafeMemory::IsValidReadPtr(pR3, 5) && pR3[0] == 0x55 && pR3[1] == 0x8B && pR3[2] == 0xEC && pR3[3] == 0x83 && pR3[4] == 0xEC)
                {
                    s_Version = Version::R3;
                    s_Config = { "0.3.7-R3 (Signature)", 0x26E8DC, 0x26E8F4, 0x9FFE0, 0x9FEC0, 0x3DE, 0x8, 0x4, 0xFB4, 0x2F1C };
                }
                else if (pR4 && SafeMemory::IsValidReadPtr(pR4, 5) && pR4[0] == 0x55 && pR4[1] == 0x8B && pR4[2] == 0xEC && pR4[3] == 0x83 && pR4[4] == 0xEC)
                {
                    s_Version = Version::R4;
                    s_Config = { "0.3.7-R4 (Signature)", 0x26EA04, 0x26EA0C, 0xA0750, 0xA0630, 0x3DE, 0x8, 0x4, 0xFB4, 0x2F1C };
                }
                else if (pR5 && SafeMemory::IsValidReadPtr(pR5, 5) && pR5[0] == 0x55 && pR5[1] == 0x8B && pR5[2] == 0xEC && pR5[3] == 0x83 && pR5[4] == 0xEC)
                {
                    s_Version = Version::R5;
                    s_Config = { "0.3.7-R5 (Signature)", 0x26EB94, 0x26EBAC, 0xA0890, 0xA0770, 0x3DE, 0x4, 0x4, 0xFB4, 0x2F1C };
                }
                else if (pDL && SafeMemory::IsValidReadPtr(pDL, 5) && pDL[0] == 0x55 && pDL[1] == 0x8B && pDL[2] == 0xEC && pDL[3] == 0x83 && pDL[4] == 0xEC)
                {
                    s_Version = Version::DL;
                    s_Config = { "0.3.DL-1 (Signature)", 0x2ACA14, 0x2ACA24, 0xA0530, 0xA0410, 0x3DE, 0x8, 0x4, 0xFB4, 0x2F1C };
                }
                else
                {
                    // Versão não comprovada: NUNCA assumir R3 cegamente!
                    s_Version = Version::Unknown;
                    s_Config = { "Unknown / Unsupported", 0, 0, 0, 0, 0, 0, 0, 0, 0 };
                    Logger::Log("[SAMP] AVISO CRITICO: Versao do SA-MP nao reconhecida (EP=0x%X, TimeStamp=0x%X). Acesso a estruturas do SAMP bloqueado.",
                        entryPoint, timeDateStamp);
                }
            }

            s_Initialized = true;
            Logger::Log("[SAMP] module loaded");
            Logger::Log("[SAMP] version detected=%s", s_Config.name);
            Logger::Log("[SAMP] base=0x%p", reinterpret_cast<void*>(sampBase));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            s_Version = Version::Unknown;
            s_Config = { "Unknown / Exception", 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            s_Initialized = true;
            Logger::Log("[SAMP] ERRO: Excecao durante deteccao de versao do SA-MP.");
        }
    }

    Version GetVersion()
    {
        DetectVersion();
        return s_Version;
    }

    const char* GetVersionString()
    {
        DetectVersion();
        return s_Config.name ? s_Config.name : "Unknown";
    }

    uintptr_t GetSAMPInfo()
    {
        DetectVersion();
        if (s_Version == Version::Unknown || s_Config.infoOffset == 0)
            return 0;

        uintptr_t sampBase = GetBaseAddress();
        if (!sampBase) return 0;

        uintptr_t pSAMP = 0;
        if (!SafeMemory::SafeRead(sampBase + s_Config.infoOffset, pSAMP))
            return 0;

        return pSAMP;
    }

    uintptr_t GetPools()
    {
        uintptr_t pSAMP = GetSAMPInfo();
        if (!pSAMP) return 0;

        uintptr_t pPools = 0;
        if (!SafeMemory::SafeRead(pSAMP + s_Config.poolsOffset, pPools))
            return 0;

        return pPools;
    }

    uintptr_t GetPlayerPool()
    {
        uintptr_t pPools = GetPools();
        if (!pPools) return 0;

        uintptr_t pPlayerPool = 0;
        if (!SafeMemory::SafeRead(pPools + s_Config.playerPoolOffset, pPlayerPool))
            return 0;

        return pPlayerPool;
    }

    uint16_t GetLocalPlayerId()
    {
        uintptr_t pPlayerPool = GetPlayerPool();
        if (!pPlayerPool) return 0xFFFF;

        uint16_t localId = 0xFFFF;
        if (!SafeMemory::SafeRead(pPlayerPool + s_Config.localPlayerIdOffset, localId))
            return 0xFFFF;

        return localId;
    }

    uintptr_t GetLocalPlayer()
    {
        if (s_Version == Version::Unknown) return 0;
        uintptr_t pPlayerPool = GetPlayerPool();
        if (!pPlayerPool) return 0;

        void* rawLocalPed = RuntimeState::GetLocalPed();
        if (!rawLocalPed)
        {
            void** ppGta = reinterpret_cast<void**>(0x00B7CD98);
            if (ppGta && SafeMemory::IsValidReadPtr(ppGta, sizeof(void*)))
                rawLocalPed = *ppGta;
        }

        __try
        {
            // Offset primário conforme versão
            uintptr_t primaryOff = 0;
            if (s_Version == Version::R1 || s_Version == Version::R2)
                primaryOff = 0x22;
            else if (s_Version == Version::R3 || s_Version == Version::R4 || s_Version == Version::R5 || s_Version == Version::DL)
                primaryOff = 0x2F1C;

            // 1. Testa primeiro o offset primário
            if (primaryOff != 0 && SafeMemory::IsValidReadPtr(reinterpret_cast<void*>(pPlayerPool + primaryOff), sizeof(void*)))
            {
                uintptr_t pCandidate = *reinterpret_cast<uintptr_t*>(pPlayerPool + primaryOff);
                if (pCandidate > 0x10000 && SafeMemory::IsValidReadPtr(reinterpret_cast<void*>(pCandidate), 64))
                {
                    if (rawLocalPed)
                    {
                        void* pInternalPed = *reinterpret_cast<void**>(pCandidate);
                        if (pInternalPed == rawLocalPed)
                            return pCandidate;
                    }
                    else
                    {
                        return pCandidate;
                    }
                }
            }

            // 2. Probing secundário validando estritamente com o ped nativo
            if (rawLocalPed)
            {
                static const uintptr_t candidateOffsets[] = {
                    0x2F1C, 0x22, 0x2F38, 0x2F3C, 0x2F20, 0x20, 0x24, 0x18, 0x2E
                };

                for (uintptr_t off : candidateOffsets)
                {
                    if (!SafeMemory::IsValidReadPtr(reinterpret_cast<void*>(pPlayerPool + off), sizeof(void*)))
                        continue;

                    uintptr_t pCandidate = *reinterpret_cast<uintptr_t*>(pPlayerPool + off);
                    if (pCandidate > 0x10000 && SafeMemory::IsValidReadPtr(reinterpret_cast<void*>(pCandidate), 64))
                    {
                        void* pInternalPed = *reinterpret_cast<void**>(pCandidate);
                        if (pInternalPed == rawLocalPed)
                        {
                            return pCandidate;
                        }
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
        return 0;
    }

    uintptr_t GetLocalPlayerOnFootData()
    {
        uintptr_t pLocal = GetLocalPlayer();
        if (!pLocal) return 0;

        __try
        {
            float localPos[3] = { 0 };
            bool hasPos = GetLocalPlayerPosition(localPos);

            // Offsets conhecidos do stOnFootData dentro de CLocalPlayer
            static const uintptr_t dataOffsets[] = { 0x18, 0x1C, 0x20, 0x24, 0x4C, 0x14, 0x3DE };
            for (uintptr_t dOff : dataOffsets)
            {
                uintptr_t pData = pLocal + dOff;
                if (!SafeMemory::IsValidWritePtr(reinterpret_cast<void*>(pData), 68))
                    continue;

                if (hasPos)
                {
                    float* pPos = reinterpret_cast<float*>(pData + 6);
                    if (SafeMemory::IsValidReadPtr(pPos, 12))
                    {
                        float dx = pPos[0] - localPos[0];
                        float dy = pPos[1] - localPos[1];
                        float dz = pPos[2] - localPos[2];
                        if ((dx * dx + dy * dy + dz * dz) < 400.0f) // Tolerância de até 20m
                        {
                            return pData;
                        }
                    }
                }
                else
                {
                    return pData;
                }
            }

            // Fallback para o offset primário padrão
            return pLocal + 0x18;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
        return 0;
    }

    void ToggleCursor(bool enable)
    {
        DetectVersion();
        uintptr_t sampBase = GetBaseAddress();
        if (!sampBase || !s_Config.miscInfoOffset || !s_Config.toggleCursorOffset)
            return;

        __try
        {
            void** ppMiscInfo = reinterpret_cast<void**>(sampBase + s_Config.miscInfoOffset);
            if (ppMiscInfo && *ppMiscInfo)
            {
                void* pMiscInfo = *ppMiscInfo;
                auto fnToggleCursor = reinterpret_cast<void(__thiscall*)(void*, int, bool)>(sampBase + s_Config.toggleCursorOffset);
                auto fnUnlockCam = reinterpret_cast<void(__thiscall*)(void*)>(sampBase + s_Config.unlockCamOffset);

                if (enable)
                {
                    if (fnToggleCursor)
                    {
                        fnToggleCursor(pMiscInfo, 3, false);
                    }
                }
                else
                {
                    if (fnToggleCursor)
                    {
                        fnToggleCursor(pMiscInfo, 0, false);
                    }

                    // Limpa explicitamente os campos de bloqueio no miscInfo:
                    // field_59 (+0x59): flag que bloqueia o UnlockCam
                    // field_55 (+0x55): modo do cursor (0 = inativo)
                    // Replica com exatidão a rotina oficial de fechamento de chat do SA-MP (RVA 0x6DE7F)
                    *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(pMiscInfo) + 0x59) = 0;
                    *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(pMiscInfo) + 0x55) = 0;

                    if (fnUnlockCam)
                    {
                        fnUnlockCam(pMiscInfo);
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    bool HasActiveCursor()
    {
        DetectVersion();
        uintptr_t sampBase = GetBaseAddress();
        if (!sampBase || !s_Config.miscInfoOffset)
            return false;

        __try
        {
            void** ppMiscInfo = reinterpret_cast<void**>(sampBase + s_Config.miscInfoOffset);
            if (ppMiscInfo && *ppMiscInfo)
            {
                uint32_t mode = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(*ppMiscInfo) + 0x55);
                return (mode != 0);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
        return false;
    }

    bool GetRemotePlayer(int index, RemotePlayerData& outData)
    {
        outData.playerId = index;
        outData.isValid = false;
        outData.isStreamed = false;
        outData.gtaPedHandle = 0;
        outData.pGtaPed = nullptr;
        outData.position[0] = 0.0f;
        outData.position[1] = 0.0f;
        outData.position[2] = 0.0f;
        outData.health = 0.0f;
        outData.armor = 0.0f;
        outData.name[0] = '\0';
        outData.color = 0;
        outData.team = -1;

        uintptr_t pPlayerPool = GetPlayerPool();
        if (!pPlayerPool || index < 0 || index >= 1004)
            return false;

        __try
        {
            // 1. Checa se o jogador está listado (m_bNotEmpty / iIsListed)
            int isListed = *reinterpret_cast<int*>(pPlayerPool + s_Config.isListedOffset + index * 4);
            if (isListed != 1)
                return false;

            // 2. Obtém ponteiro do RemotePlayer
            uintptr_t pRemotePlayer = *reinterpret_cast<uintptr_t*>(pPlayerPool + s_Config.remotePlayerOffset + index * 4);
            if (!pRemotePlayer)
                return false;

            outData.isValid = true;

            // 2.1 Lê cor e team do jogador remoto
            if (SafeMemory::IsValidReadPtr(reinterpret_cast<void*>(pRemotePlayer + 0x28), sizeof(uint32_t)))
            {
                outData.color = *reinterpret_cast<uint32_t*>(pRemotePlayer + 0x28);
            }
            if (SafeMemory::IsValidReadPtr(reinterpret_cast<void*>(pRemotePlayer + 0x8), sizeof(uint8_t)))
            {
                outData.team = static_cast<int>(*reinterpret_cast<uint8_t*>(pRemotePlayer + 0x8));
            }

            // 3. Lê o Nome do Jogador
            const char* pName = reinterpret_cast<const char*>(pRemotePlayer + 0xC);
            if (pName && pName[0] != '\0')
            {
                size_t nameRes = *reinterpret_cast<size_t*>(pRemotePlayer + 0x1C);
                if (nameRes >= 16)
                {
                    const char* pHeapName = *reinterpret_cast<const char**>(pRemotePlayer + 0xC);
                    if (pHeapName && pHeapName[0] != '\0')
                    {
                        strncpy(outData.name, pHeapName, sizeof(outData.name) - 1);
                    }
                    else
                    {
                        strncpy(outData.name, pName, sizeof(outData.name) - 1);
                    }
                }
                else
                {
                    strncpy(outData.name, pName, sizeof(outData.name) - 1);
                }
            }
            outData.name[sizeof(outData.name) - 1] = '\0';

            // 4. Obtém pPlayerData (offset 0x0)
            uintptr_t pPlayerData = *reinterpret_cast<uintptr_t*>(pRemotePlayer + 0x0);
            if (!pPlayerData)
                return true;

            // 5. Obtém pSAMP_Actor (offset 0x0 de pPlayerData)
            uintptr_t pSAMP_Actor = *reinterpret_cast<uintptr_t*>(pPlayerData + 0x0);
            if (!pSAMP_Actor)
                return true;

            outData.isStreamed = true;

            // 6. Obtém o handle do GTA Entity
            uint32_t handle = *reinterpret_cast<uint32_t*>(pSAMP_Actor + 0x44);
            void* pGtaPed = GTA::GetPed(handle);
            if (!pGtaPed)
            {
                handle = *reinterpret_cast<uint32_t*>(pSAMP_Actor + 0x48);
                pGtaPed = GTA::GetPed(handle);
            }
            if (!pGtaPed)
            {
                pGtaPed = *reinterpret_cast<void**>(pSAMP_Actor + 0x40);
            }
            if (!pGtaPed)
            {
                pGtaPed = *reinterpret_cast<void**>(pSAMP_Actor + 0x2A4);
            }

            outData.gtaPedHandle = handle;
            outData.pGtaPed = pGtaPed;

            if (pGtaPed)
            {
                GTA::GetPedPosition(pGtaPed, outData.position);
                outData.health = GTA::GetPedHealth(pGtaPed);
                outData.armor  = GTA::GetPedArmor(pGtaPed);
            }
            else
            {
                float* pPos = reinterpret_cast<float*>(pPlayerData + (s_Version == Version::R1 ? 0x7B : 0x58));
                if (pPos)
                {
                    outData.position[0] = pPos[0];
                    outData.position[1] = pPos[1];
                    outData.position[2] = pPos[2];
                }
            }

            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool GetLocalPlayerPosition(float outPos[3])
    {
        __try
        {
            void* pLocalPed = *reinterpret_cast<void**>(0x00B7CD98);
            if (pLocalPed)
            {
                return GTA::GetPedPosition(pLocalPed, outPos);
            }
            return false;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    uint32_t GetLocalPlayerColor()
    {
        uintptr_t pPlayerPool = GetPlayerPool();
        if (!pPlayerPool) return 0;

        __try
        {
            uintptr_t colorOff = (s_Version == Version::R1) ? 0x8 : 0x10;
            if (SafeMemory::IsValidReadPtr(reinterpret_cast<void*>(pPlayerPool + colorOff), sizeof(uint32_t)))
            {
                return *reinterpret_cast<uint32_t*>(pPlayerPool + colorOff);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return 0;
    }

    bool IsTeammate(int index)
    {
        if (index < 0 || index >= 1004)
            return false;

        uint16_t localId = GetLocalPlayerId();
        if (index == static_cast<int>(localId))
            return true;

        RemotePlayerData rp;
        if (!GetRemotePlayer(index, rp) || !rp.isValid)
            return false;

        // Se o servidor definiu Team (diferente de 255 e -1)
        uint8_t localTeam = 255;
        uintptr_t pLocal = GetLocalPlayer();
        if (pLocal && SafeMemory::IsValidReadPtr(reinterpret_cast<void*>(pLocal + 0x8), sizeof(uint8_t)))
        {
            localTeam = *reinterpret_cast<uint8_t*>(pLocal + 0x8);
        }

        if (localTeam != 255 && rp.team != 255 && rp.team != -1)
        {
            return (rp.team == static_cast<int>(localTeam));
        }

        // Se time não estiver configurado pelo servidor, confronta cor se não for nula/branca padrão
        uint32_t localColor = GetLocalPlayerColor();
        if (localColor != 0 && rp.color != 0 && (rp.color & 0x00FFFFFF) == (localColor & 0x00FFFFFF))
        {
            return true;
        }

        return false;
    }

    typedef bool(__thiscall* SendBitStream_t)(void* pThis, void* pBitStream, int priority, int reliability, char orderingChannel);
    typedef bool(__thiscall* SendData_t)(void* pThis, const char* data, int length, int priority, int reliability, char orderingChannel);

    static SendBitStream_t s_OriginalSendBitStream = nullptr;
    static SendData_t s_OriginalSendData = nullptr;
    static void** s_HookedVTable = nullptr;
    static uintptr_t s_HookedOffset = 0;
    static bool s_RakHookInstalled = false;

    static void MutateInvertebredPacket(unsigned char* data, int length)
    {
        if (!data || length < 68) return;

        // data[0] == 207 (0xCF) -> ID_PLAYER_SYNC (OnFoot Sync)
        if (data[0] != 207) return;

        // 1. Desincroniza Quaternion (fQuaternion[0..3]) nos bytes 19 a 34 (4 floats)
        float* pQuat = reinterpret_cast<float*>(data + 19);
        pQuat[0] = static_cast<float>(rand() % 256);
        pQuat[1] = static_cast<float>(rand() % 256);
        pQuat[2] = static_cast<float>(rand() % 256);
        pQuat[3] = static_cast<float>(rand() % 256);

        // 2. Desincroniza ID de animacao (byte 65) e flags (byte 67) exatamente como TwistPlayer.cs
        static const uint16_t s_animIds[] = { 0x0B03, 0x0477, 0x045D, 0x0443, 0x0429 };
        *reinterpret_cast<uint16_t*>(data + 65) = s_animIds[rand() % 5];
        *reinterpret_cast<uint16_t*>(data + 67) = 12082; // Flag de animacao do Blume
    }

    // ─────────────────────────────────────────────────────────────
    // IMPLEMENTAÇÃO DO SILENT AIM COM INSTRUMENTAÇÃO DE DIAGNÓSTICO
    // ─────────────────────────────────────────────────────────────

    static int SilentResolveBoneIndex(int menuBoneOption)
    {
        // menuBoneOption: 0=HEAD, 1=NECK, 2=CHEST, 3=PELVIS, 4=RANDOM
        switch (menuBoneOption)
        {
        case 0: return 8;  // HEAD
        case 1: return 5;  // NECK
        case 2: return 4;  // CHEST
        case 3: return 2;  // PELVIS
        case 4:           // RANDOM (apenas Silent)
        {
            int bones[] = { 8, 5, 4, 2 };
            return bones[rand() % 4];
        }
        default: return 8;
        }
    }

    static const char* SilentBoneName(int boneId)
    {
        switch (boneId)
        {
        case 8: return "HEAD";
        case 5: return "NECK";
        case 4: return "CHEST";
        case 2: return "PELVIS";
        default: return "HEAD";
        }
    }

    static bool SilentGetEyePosition(float outEye[3])
    {
        outEye[0] = 0.0f;
        outEye[1] = 0.0f;
        outEye[2] = 0.0f;

        void* pLocalPed = *reinterpret_cast<void**>(0x00B7CD98);
        if (!pLocalPed) return false;

        __try
        {
            // Pega a posição da cabeça (osso 8) = origem dos olhos / arma
            if (GTA::GetPedBonePosition(pLocalPed, 8, outEye))
            {
                return true;
            }

            // Fallback: posição do ped + offset de altura média
            float pedPos[3] = { 0 };
            if (GTA::GetPedPosition(pLocalPed, pedPos))
            {
                outEye[0] = pedPos[0];
                outEye[1] = pedPos[1];
                outEye[2] = pedPos[2] + 0.72f; // Altura média dos olhos em pé
                return true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return false;
    }

    struct SilentShotTarget
    {
        bool valid = false;
        int playerId = -1;
        float boneWorldPos[3] = { 0 };
        float pedCenterPos[3] = { 0 };
        int boneId = 8;
        char boneName[16] = "HEAD";
        char playerName[32] = "";
    };

    // Executado NO MOMENTO DO DISPARO com diagnóstico detalhado de cada ponto de saída
    static SilentShotTarget SilentFindTargetOnShot()
    {
        SilentShotTarget result = {};

        // ── Critério 1: Jogador vivo ──
        if (!RuntimeState::IsPlayerAlive())
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: Jogador local morto ou ped invalido");
            return result;
        }

        // ── Critério 2: Silent master enable ──
        if (!g_MenuState.silentAim.enabled)
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: g_MenuState.silentAim.enabled == false");
            return result;
        }

        // ── Critério 3: Grupo de arma e perfil ──
        int activeGroup = Aimbot::GetActiveWeaponGroup();
        if (activeGroup < 0 || activeGroup >= 4) activeGroup = 0;
        const auto& sw = g_MenuState.silentAim.weapons[activeGroup];
        if (!sw.enabled)
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: Perfil de arma desativado (grupo=%d, armaID=%u)",
                activeGroup, Aimbot::GetCurrentWeaponId());
            return result;
        }

        // ── Critério 4: Modo de ativação (chaves no momento do disparo) ──
        bool isAiming = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        bool isShooting = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0) ||
                          ((GetTickCount64() - AimAssist::GetLastLocalShotTick()) < 150);
        bool activationOK = false;
        switch (sw.activationMode)
        {
        case 0: activationOK = true; break;                              // Always
        case 1: activationOK = isAiming; break;                          // While Aiming (RMB)
        case 2: activationOK = isShooting; break;                        // While Shooting (LMB)
        case 3: activationOK = (isAiming && isShooting); break;          // Aim + Shoot
        default: activationOK = isShooting; break;
        }
        if (!activationOK)
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: Condicao de ativacao nao atendida (mode=%d, isAiming=%d, isShooting=%d)",
                sw.activationMode, isAiming ? 1 : 0, isShooting ? 1 : 0);
            return result;
        }

        // ── Critério 5: Hit Chance ──
        if (sw.hitChance < 100)
        {
            int roll = rand() % 100;
            if (roll >= sw.hitChance)
            {
                Logger::Log("[SILENT][DIAG][FAIL] Motivo: HitChance falhou (roll=%d >= config=%d)", roll, sw.hitChance);
                return result;
            }
        }

        // ── Critério 6: SAMP Carregado ──
        if (!IsLoaded())
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: SAMP nao esta carregado");
            return result;
        }

        float localEye[3] = { 0 };
        SilentGetEyePosition(localEye);

        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        if (displaySize.x <= 0 || displaySize.y <= 0)
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: ImGui DisplaySize invalido (%.0f, %.0f)", displaySize.x, displaySize.y);
            return result;
        }
        ImVec2 screenCenter(displaySize.x * 0.5f, displaySize.y * 0.5f);
        float fovRadius = Aimbot::GetFovRadius(sw.fov);

        // ── Bone ID no momento do disparo (resolve RANDOM também) ──
        int realBoneId = SilentResolveBoneIndex(sw.bone);
        const char* realBoneName = SilentBoneName(realBoneId);

        // ── Target Selection FRESH (não usa cache do render loop) ──
        WeaponAimConfig selectorCfg;
        selectorCfg.bone = sw.bone;
        selectorCfg.maxDistance = sw.maxDistance;
        selectorCfg.priority = sw.priority;
        selectorCfg.ignoreDead = sw.ignoreDead;
        selectorCfg.teamCheck = sw.teamCheck;
        selectorCfg.visibilityCheck = sw.visibilityCheck;
        selectorCfg.fov = sw.fov;

        int candidates = 0;
        int insideFov = 0;
        TargetInfo freshTarget = TargetSelector::FindBestTarget(selectorCfg, screenCenter, fovRadius, candidates, insideFov);

        if (!freshTarget.valid || freshTarget.playerId < 0)
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: TargetSelector nao encontrou alvo (candidatos=%d, dentroFOV=%d, fovRaio=%.1f px, maxDist=%.1fm)",
                candidates, insideFov, fovRadius, sw.maxDistance);
            return result;
        }

        // ── Validação FINAL: jogador ainda existe na POOL no momento do tiro ──
        RemotePlayerData rpData;
        if (!GetRemotePlayer(freshTarget.playerId, rpData) || !rpData.isValid)
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: RemotePlayer #%d invalido na pool SAMP", freshTarget.playerId);
            return result;
        }

        if (!rpData.isStreamed || !rpData.pGtaPed)
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: RemotePlayer #%d (%s) nao esta streamed ou ped nulo",
                freshTarget.playerId, rpData.name);
            return result;
        }

        if (!RuntimeState::IsValidPed(rpData.pGtaPed))
        {
            Logger::Log("[SILENT][DIAG][FAIL] Motivo: Ped do RemotePlayer #%d (%s) falhou em IsValidPed",
                freshTarget.playerId, rpData.name);
            return result;
        }

        if (sw.ignoreDead)
        {
            if (!GTA::IsPedAlive(rpData.pGtaPed) || rpData.health <= 0.0f)
            {
                Logger::Log("[SILENT][DIAG][FAIL] Motivo: RemotePlayer #%d (%s) esta morto (health=%.1f)",
                    freshTarget.playerId, rpData.name, rpData.health);
                return result;
            }
        }

        // ── Re-obtém a posição 3D do OSSO EXATO agora ──
        float finalBonePos[3] = { 0 };
        bool gotBone = false;
        __try
        {
            gotBone = GTA::GetPedBonePosition(rpData.pGtaPed, realBoneId, finalBonePos);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { gotBone = false; }

        if (!gotBone)
        {
            finalBonePos[0] = rpData.position[0];
            finalBonePos[1] = rpData.position[1];
            float addZ = 0.2f;
            if (realBoneId == 8) addZ = 0.82f;
            else if (realBoneId == 5) addZ = 0.65f;
            else if (realBoneId == 4) addZ = 0.35f;
            finalBonePos[2] = rpData.position[2] + addZ;
        }

        // ── Preenche resultado válido ──
        result.valid = true;
        result.playerId = freshTarget.playerId;
        result.boneId = realBoneId;
        result.boneWorldPos[0] = finalBonePos[0];
        result.boneWorldPos[1] = finalBonePos[1];
        result.boneWorldPos[2] = finalBonePos[2];
        result.pedCenterPos[0] = rpData.position[0];
        result.pedCenterPos[1] = rpData.position[1];
        result.pedCenterPos[2] = rpData.position[2];
        strncpy(result.boneName, realBoneName, sizeof(result.boneName) - 1);
        strncpy(result.playerName, rpData.name[0] ? rpData.name : "Player", sizeof(result.playerName) - 1);

        Logger::Log("[SILENT][DIAG][SUCCESS] Alvo travado: ID #%d (%s) | Osso: %s | Pos3D: (%.1f, %.1f, %.1f)",
            result.playerId, result.playerName, result.boneName,
            result.boneWorldPos[0], result.boneWorldPos[1], result.boneWorldPos[2]);

        return result;
    }

    static void MutateBulletSyncPacket(unsigned char* data, int length)
    {
        if (!data || length < 40)
        {
            Logger::Log("[SILENT][DIAG][MUTATE] MutateBulletSyncPacket abortado: data=%p length=%d (<40)", data, length);
            return;
        }
        if (data[0] != 206)
        {
            Logger::Log("[SILENT][DIAG][MUTATE] MutateBulletSyncPacket abortado: data[0]=%u (!=206)", data[0]);
            return;
        }

        Logger::Log("[SILENT][DIAG][MUTATE] MutateBulletSyncPacket iniciado (packetId=206, length=%d)", length);

        // ── Validação e Seleção no momento do disparo ──
        SilentShotTarget shot = SilentFindTargetOnShot();
        if (!shot.valid)
        {
            Logger::Log("[SILENT][DIAG][MUTATE] SilentFindTargetOnShot retornou INVALIDO -> Disparo original mantido.");
            return;
        }

        // ── Origem do tiro: posição dos olhos / arma do jogador LOCAL ──
        float eyePos[3] = { 0 };
        SilentGetEyePosition(eyePos);
        *reinterpret_cast<float*>(data + 4)  = eyePos[0];  // fOrigin[0]
        *reinterpret_cast<float*>(data + 8)  = eyePos[1];  // fOrigin[1]
        *reinterpret_cast<float*>(data + 12) = eyePos[2];  // fOrigin[2]

        // ── Tipo de hit: Player (1) ──
        data[1] = 1; // byteType = BULLET_HIT_TYPE_PLAYER

        // ── ID do jogador alvo ──
        *reinterpret_cast<uint16_t*>(data + 2) = static_cast<uint16_t>(shot.playerId);

        // ── fTarget[3]: coordenadas 3D do OSSO do alvo ──
        *reinterpret_cast<float*>(data + 16) = shot.boneWorldPos[0];
        *reinterpret_cast<float*>(data + 20) = shot.boneWorldPos[1];
        *reinterpret_cast<float*>(data + 24) = shot.boneWorldPos[2];

        // ── fCenter[3]: deslocamento RELATIVO do osso em relação ao centro do ped alvo ──
        *reinterpret_cast<float*>(data + 28) = shot.boneWorldPos[0] - shot.pedCenterPos[0];
        *reinterpret_cast<float*>(data + 32) = shot.boneWorldPos[1] - shot.pedCenterPos[1];
        *reinterpret_cast<float*>(data + 36) = shot.boneWorldPos[2] - shot.pedCenterPos[2];

        float dx = shot.boneWorldPos[0] - eyePos[0];
        float dy = shot.boneWorldPos[1] - eyePos[1];
        float dz = shot.boneWorldPos[2] - eyePos[2];
        float dist3D = sqrtf(dx*dx + dy*dy + dz*dz);
        Logger::Log("[SILENT][DIAG][REDIRECT] SHOT REDIRECTED -> target=%d (%s) bone=%s dist=%.1fm origin=(%.1f,%.1f,%.1f) targetBone=(%.1f,%.1f,%.1f)",
            shot.playerId, shot.playerName, shot.boneName, dist3D,
            eyePos[0], eyePos[1], eyePos[2],
            shot.boneWorldPos[0], shot.boneWorldPos[1], shot.boneWorldPos[2]);
    }

    static bool __fastcall Hooked_SendBitStream(void* pThis, void* edx, void* pBitStream, int priority, int reliability, char orderingChannel)
    {
        Main::CallbackGuard guard;

        if (!s_OriginalSendBitStream)
        {
            return false;
        }

        if (!guard.IsActive())
        {
            return s_OriginalSendBitStream(pThis, pBitStream, priority, reliability, orderingChannel);
        }

        if (pBitStream && SafeMemory::IsValidReadPtr(pBitStream, 16))
        {
            int numberOfBitsUsed = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(pBitStream) + 0);
            int byteCount = (numberOfBitsUsed + 7) / 8;
            unsigned char* data = *reinterpret_cast<unsigned char**>(reinterpret_cast<uintptr_t>(pBitStream) + 12);
            if (data && byteCount >= 1)
            {
                unsigned char packetId = data[0];

                // Diagnóstico durante disparo (LMB pressionado) ou para pacotes de sync/combate
                bool isShooting = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                if (packetId == 206)
                {
                    Logger::Log("[SAMP][DIAG][NET] Hooked_SendBitStream: ID_BULLET_SYNC (206) interceptado! byteCount=%d (bits=%d)",
                        byteCount, numberOfBitsUsed);
                    MutateBulletSyncPacket(data, byteCount);
                }
                else if (isShooting && (packetId == 207 || packetId == 211 || packetId == 212))
                {
                    static uint64_t s_lastCombatBitLog = 0;
                    uint64_t now = GetTickCount64();
                    if (now - s_lastCombatBitLog >= 300)
                    {
                        Logger::Log("[SAMP][DIAG][NET] Hooked_SendBitStream durante disparo: packetId=%u byteCount=%d",
                            packetId, byteCount);
                        s_lastCombatBitLog = now;
                    }
                }

                if (packetId == 207 && byteCount >= 68)
                {
                    if (g_MenuState.antiAim.invertebred)
                    {
                        MutateInvertebredPacket(data, byteCount);
                    }
                }

                // Fake Lag: represa pacotes de sincronização de jogador (207) ou mira (203)
                if ((packetId == 207 || packetId == 203) && AntiAim::ShouldChokeSyncPacket())
                {
                    return true;
                }
            }
        }
        return s_OriginalSendBitStream(pThis, pBitStream, priority, reliability, orderingChannel);
    }

    static bool __fastcall Hooked_SendData(void* pThis, void* edx, const char* data, int length, int priority, int reliability, char orderingChannel)
    {
        Main::CallbackGuard guard;

        if (!s_OriginalSendData)
        {
            return false;
        }

        if (!guard.IsActive())
        {
            return s_OriginalSendData(pThis, data, length, priority, reliability, orderingChannel);
        }

        if (data && length >= 1)
        {
            unsigned char packetId = static_cast<unsigned char>(data[0]);

            bool isShooting = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            if (packetId == 206)
            {
                Logger::Log("[SAMP][DIAG][NET] Hooked_SendData: ID_BULLET_SYNC (206) interceptado! length=%d", length);
                if (SafeMemory::IsValidWritePtr(const_cast<char*>(data), length))
                {
                    MutateBulletSyncPacket(reinterpret_cast<unsigned char*>(const_cast<char*>(data)), length);
                }
            }
            else if (isShooting && (packetId == 207 || packetId == 211 || packetId == 212))
            {
                static uint64_t s_lastCombatDataLog = 0;
                uint64_t now = GetTickCount64();
                if (now - s_lastCombatDataLog >= 300)
                {
                    Logger::Log("[SAMP][DIAG][NET] Hooked_SendData durante disparo: packetId=%u length=%d",
                        packetId, length);
                    s_lastCombatDataLog = now;
                }
            }

            if (packetId == 207 && length >= 68)
            {
                if (g_MenuState.antiAim.invertebred)
                {
                    if (SafeMemory::IsValidWritePtr(const_cast<char*>(data), length))
                    {
                        MutateInvertebredPacket(reinterpret_cast<unsigned char*>(const_cast<char*>(data)), length);
                    }
                }
            }

            // Fake Lag: represa pacotes de sincronização de jogador (207) ou mira (203)
            if ((packetId == 207 || packetId == 203) && AntiAim::ShouldChokeSyncPacket())
            {
                return true;
            }
        }
        return s_OriginalSendData(pThis, data, length, priority, reliability, orderingChannel);
    }

    bool EnsureRakHook()
    {
        if (Main::IsShuttingDown()) return false;
        if (s_RakHookInstalled) return true;

        uintptr_t sampBase = GetBaseAddress();
        if (!sampBase)
        {
            static uint64_t s_lastBaseFailLog = 0;
            uint64_t now = GetTickCount64();
            if (now - s_lastBaseFailLog >= 4000)
            {
                Logger::Log("[SAMP][DIAG][HOOK_FAIL] EnsureRakHook: samp.dll base address is NULL (modulo nao carregado)");
                s_lastBaseFailLog = now;
            }
            return false;
        }

        uintptr_t sampInfo = GetSAMPInfo();
        if (!sampInfo)
        {
            static uint64_t s_lastInfoFailLog = 0;
            uint64_t now = GetTickCount64();
            if (now - s_lastInfoFailLog >= 4000)
            {
                Logger::Log("[SAMP][DIAG][HOOK_FAIL] EnsureRakHook: GetSAMPInfo() retornou 0 (versao=%s, infoOffset=0x%X)",
                    GetVersionString(), s_Config.infoOffset);
                s_lastInfoFailLog = now;
            }
            return false;
        }

        __try
        {
            uintptr_t directOff = (s_Config.poolsOffset > 4) ? (s_Config.poolsOffset - 4) : 0x3DA;
            static const uintptr_t candidateOffsets[] = {
                directOff, 0x3DA, 0x3C9, 0x3D8, 0x3D6, 0x3DC, 0x3E2, 0x2C
            };

            for (uintptr_t off : candidateOffsets)
            {
                if (!SafeMemory::IsValidReadPtr(reinterpret_cast<void*>(sampInfo + off), sizeof(void*)))
                    continue;

                uintptr_t pCandidate = *reinterpret_cast<uintptr_t*>(sampInfo + off);
                if (pCandidate > 0x10000 && SafeMemory::IsValidReadPtr(reinterpret_cast<void*>(pCandidate), sizeof(void*)))
                {
                    void** vtable = *reinterpret_cast<void***>(pCandidate);
                    if (SafeMemory::IsValidReadPtr(vtable, sizeof(void*) * 10))
                    {
                        uintptr_t fn6 = reinterpret_cast<uintptr_t>(vtable[6]);
                        uintptr_t fn7 = reinterpret_cast<uintptr_t>(vtable[7]);
                        if (fn6 >= sampBase && fn6 < (sampBase + 0x400000) &&
                            fn7 >= sampBase && fn7 < (sampBase + 0x400000))
                        {
                            DWORD oldProtect = 0;
                            if (VirtualProtect(&vtable[6], sizeof(void*) * 2, PAGE_EXECUTE_READWRITE, &oldProtect))
                            {
                                s_OriginalSendData = reinterpret_cast<SendData_t>(vtable[6]);
                                s_OriginalSendBitStream = reinterpret_cast<SendBitStream_t>(vtable[7]);

                                vtable[6] = reinterpret_cast<void*>(&Hooked_SendData);
                                vtable[7] = reinterpret_cast<void*>(&Hooked_SendBitStream);

                                VirtualProtect(&vtable[6], sizeof(void*) * 2, oldProtect, &oldProtect);

                                s_HookedVTable = vtable;
                                s_HookedOffset = off;
                                s_RakHookInstalled = true;
                                Logger::Log("[SAMP][DIAG][HOOK_SUCCESS] RakClient VMT hook instalado com sucesso! Offset=0x%X, VTable=%p, origSendData=%p, origSendBitStream=%p",
                                    off, vtable, s_OriginalSendData, s_OriginalSendBitStream);
                                return true;
                            }
                            else
                            {
                                Logger::Log("[SAMP][DIAG][HOOK_FAIL] VirtualProtect falhou ao alterar protecao da VTable no offset 0x%X", off);
                            }
                        }
                    }
                }
            }

            static uint64_t s_lastNoCandLog = 0;
            uint64_t now = GetTickCount64();
            if (now - s_lastNoCandLog >= 4000)
            {
                Logger::Log("[SAMP][DIAG][HOOK_FAIL] Nao foi encontrada VTable valida do RakClient nos offsets testados (sampBase=%p, sampInfo=%p)",
                    reinterpret_cast<void*>(sampBase), reinterpret_cast<void*>(sampInfo));
                s_lastNoCandLog = now;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            Logger::Log("[SAMP][DIAG][HOOK_FAIL] Excecao SEH durante instalacao do hook RakClient");
        }
        return false;
    }

    bool IsRakHooked()
    {
        return s_RakHookInstalled;
    }

    static TeardownStatus s_TeardownStatus = TeardownStatus::NotHooked;

    TeardownStatus GetTeardownStatus()
    {
        return s_TeardownStatus;
    }

    TeardownStatus Shutdown()
    {
        if (!s_RakHookInstalled)
        {
            s_TeardownStatus = TeardownStatus::NotHooked;
            return s_TeardownStatus;
        }

        bool restored = false;

        // 1. Tenta restaurar diretamente pela VTable cacheada
        if (s_HookedVTable && SafeMemory::IsValidWritePtr(s_HookedVTable, sizeof(void*) * 10))
        {
            __try
            {
                bool isOurHook = (s_HookedVTable[6] == reinterpret_cast<void*>(&Hooked_SendData)) ||
                                 (s_HookedVTable[7] == reinterpret_cast<void*>(&Hooked_SendBitStream));
                if (isOurHook && s_OriginalSendData && s_OriginalSendBitStream)
                {
                    DWORD oldProtect = 0;
                    if (VirtualProtect(&s_HookedVTable[6], sizeof(void*) * 2, PAGE_EXECUTE_READWRITE, &oldProtect))
                    {
                        s_HookedVTable[6] = reinterpret_cast<void*>(s_OriginalSendData);
                        s_HookedVTable[7] = reinterpret_cast<void*>(s_OriginalSendBitStream);
                        VirtualProtect(&s_HookedVTable[6], sizeof(void*) * 2, oldProtect, &oldProtect);
                        restored = true;
                        Logger::Log("[SAMP][DIAG][UNHOOK] RakClient VMT hook restaurado via cached VTable (offset 0x%X)", s_HookedOffset);
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        // 2. Fallback: Se não restaurou pela vtable cacheada, varre os offsets candidatos
        if (!restored)
        {
            uintptr_t sampInfo = GetSAMPInfo();
            if (sampInfo && s_OriginalSendData && s_OriginalSendBitStream)
            {
                __try
                {
                    uintptr_t directOff = (s_Config.poolsOffset > 4) ? (s_Config.poolsOffset - 4) : 0x3DA;
                    static const uintptr_t candidateOffsets[] = {
                        directOff, 0x3DA, 0x3C9, 0x3D8, 0x3D6, 0x3DC, 0x3E2, 0x2C
                    };

                    for (uintptr_t off : candidateOffsets)
                    {
                        if (!SafeMemory::IsValidReadPtr(reinterpret_cast<void*>(sampInfo + off), sizeof(void*)))
                            continue;

                        uintptr_t pCandidate = *reinterpret_cast<uintptr_t*>(sampInfo + off);
                        if (pCandidate <= 0x10000 || !SafeMemory::IsValidReadPtr(reinterpret_cast<void*>(pCandidate), sizeof(void*)))
                            continue;

                        void** vtable = *reinterpret_cast<void***>(pCandidate);
                        if (!vtable || !SafeMemory::IsValidReadPtr(vtable, sizeof(void*) * 10))
                            continue;

                        bool isOurHook = (vtable[6] == reinterpret_cast<void*>(&Hooked_SendData)) ||
                                         (vtable[7] == reinterpret_cast<void*>(&Hooked_SendBitStream));
                        if (isOurHook)
                        {
                            DWORD oldProtect = 0;
                            if (VirtualProtect(&vtable[6], sizeof(void*) * 2, PAGE_EXECUTE_READWRITE, &oldProtect))
                            {
                                vtable[6] = reinterpret_cast<void*>(s_OriginalSendData);
                                vtable[7] = reinterpret_cast<void*>(s_OriginalSendBitStream);
                                VirtualProtect(&vtable[6], sizeof(void*) * 2, oldProtect, &oldProtect);
                                restored = true;
                                Logger::Log("[SAMP][DIAG][UNHOOK] RakClient VMT hook restaurado via scan de fallback no offset 0x%X", off);
                            }
                            break;
                        }
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {}
            }
        }

        // 3. Validação de segurança e definição de estado determinístico
        if (restored)
        {
            s_HookedVTable = nullptr;
            s_HookedOffset = 0;
            s_RakHookInstalled = false;
            s_TeardownStatus = TeardownStatus::Restored;
            Logger::Log("[SAMP][DIAG][UNHOOK] RakClient VMT hook completamente removido (RESTORED).");
        }
        else
        {
            if (s_OriginalSendData && s_OriginalSendBitStream)
            {
                s_TeardownStatus = TeardownStatus::FailedSafe;
                Logger::Log("[SAMP][DIAG][AVISO] Falha ao restaurar VTable do RakClient no Shutdown (FAILED_SAFE). Ponteiros originais preservados.");
            }
            else
            {
                s_TeardownStatus = TeardownStatus::FailedUnsafe;
                Logger::Log("[SAMP][DIAG][ERRO] Falha ao restaurar VTable do RakClient no Shutdown (FAILED_UNSAFE). Ponteiros corrompidos ou nulos.");
            }
        }

        return s_TeardownStatus;
    }
}
