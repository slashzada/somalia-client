#include "SAMP.h"
#include <atomic>
#include "../../Core/Main.h"
#include "../GTA/GTA.h"
#include "../../Core/Logger.h"
#include "../../Features/SilentAim/SilentAim.h"
#include "../../Features/AntiAim/AntiAim.h"
#include "../../Features/PlayerSlap/PlayerSlap.h"
#include "../../Core/RuntimeState.h"
#include "../../Config/Config.h"
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
        uintptr_t sendTakeDamageOffset;
        uintptr_t sendGiveDamageOffset;
        uintptr_t sendBulletDataOffset;
    };

    static Version s_Version = Version::Unknown;
    static VersionConfig s_Config = {};
    static bool s_Initialized = false;

    typedef uint32_t (__stdcall *GetColorForPlayer_t)(int playerId);
    static GetColorForPlayer_t s_fnGetColorForPlayer = nullptr;

    static uintptr_t s_sampBase = 0;

    bool IsLoaded()
    {
        if (!s_sampBase)
            s_sampBase = reinterpret_cast<uintptr_t>(GetModuleHandleA("samp.dll"));
        return (s_sampBase != 0);
    }

    uintptr_t GetBaseAddress()
    {
        if (!s_sampBase)
            s_sampBase = reinterpret_cast<uintptr_t>(GetModuleHandleA("samp.dll"));
        return s_sampBase;
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
            if (!pDos || IsBadReadPtr(pDos, sizeof(IMAGE_DOS_HEADER)) || pDos->e_magic != IMAGE_DOS_SIGNATURE)
                return;

            PIMAGE_NT_HEADERS pNt = reinterpret_cast<PIMAGE_NT_HEADERS>(sampBase + pDos->e_lfanew);
            if (!pNt || IsBadReadPtr(pNt, sizeof(IMAGE_NT_HEADERS)) || pNt->Signature != IMAGE_NT_SIGNATURE)
                return;

            DWORD entryPoint = pNt->OptionalHeader.AddressOfEntryPoint;
            DWORD timeDateStamp = pNt->FileHeader.TimeDateStamp;

            // Verificação determinística por campos oficiais do PE Header
            // 0.3.7-R1: EP 0x31DF13, TimeDateStamp 0x554D0DE8
            if (entryPoint == 0x31DF13 || timeDateStamp == 0x554D0DE8)
            {
                s_Version = Version::R1;
                s_Config = { "0.3.7-R1", 0x21A0F8, 0x21A10C, 0x9BD30, 0x9BC10, 0x3CD, 0x18, 0x2E, 0xFDE, 0x4, 0x6660, 0x6770, 0x6980 };
            }
            // 0.3.7-R2: EP 0x3195DD, TimeDateStamp 0x559D040A
            else if (entryPoint == 0x3195DD || timeDateStamp == 0x559D040A)
            {
                s_Version = Version::R2;
                s_Config = { "0.3.7-R2", 0x21A0F8, 0x21A10C, 0x9BD30, 0x9BC10, 0x3CD, 0x18, 0x2E, 0xFDE, 0x4, 0x6660, 0x6770, 0x6980 };
            }
            // 0.3.7-R3: EP 0xCC4D0, TimeDateStamp 0x5C0B4243 / 0x5C0B3E7A
            else if (entryPoint == 0xCC4D0 || timeDateStamp == 0x5C0B4243 || timeDateStamp == 0x5C0B3E7A)
            {
                s_Version = Version::R3;
                s_Config = { "0.3.7-R3", 0x26E8DC, 0x26E8F4, 0x9FFE0, 0x9FEC0, 0x3DE, 0x8, 0x4, 0xFB4, 0x2F1C, 0x6670, 0x6780, 0x6990 };
            }
            // 0.3.7-R4: EP 0xCBCB0, TimeDateStamp 0x5DE3DCB8
            else if (entryPoint == 0xCBCB0 || timeDateStamp == 0x5DE3DCB8)
            {
                s_Version = Version::R4;
                s_Config = { "0.3.7-R4", 0x26EA04, 0x26EA0C, 0xA0750, 0xA0630, 0x3DE, 0x8, 0x4, 0xFB4, 0x2F1C, 0x6670, 0x6780, 0x6990 };
            }
            // 0.3.7-R5: EP 0xCBC90, TimeDateStamp 0x6009E839
            else if (entryPoint == 0xCBC90 || timeDateStamp == 0x6009E839)
            {
                s_Version = Version::R5;
                s_Config = { "0.3.7-R5", 0x26EB94, 0x26EBAC, 0xA0890, 0xA0770, 0x3DE, 0x4, 0x4, 0xFB4, 0x2F1C, 0x6670, 0x6780, 0x6990 };
            }
            // 0.3.DL-1: EP 0xCB180, TimeDateStamp 0x5A707993
            else if (entryPoint == 0xCB180 || timeDateStamp == 0x5A707993)
            {
                s_Version = Version::DL;
                s_Config = { "0.3.DL-1", 0x2ACA14, 0x2ACA24, 0xA0530, 0xA0410, 0x3DE, 0x8, 0x4, 0xFB4, 0x2F1C, 0x6670, 0x6780, 0x6990 };
            }
            else
            {
                // Verificação secundária por byte pattern da função ToggleCursor com validação estrita (5 bytes)
                unsigned char* pR1 = reinterpret_cast<unsigned char*>(sampBase + 0x9BD30);
                unsigned char* pR3 = reinterpret_cast<unsigned char*>(sampBase + 0x9FFE0);
                unsigned char* pR4 = reinterpret_cast<unsigned char*>(sampBase + 0xA0750);
                unsigned char* pR5 = reinterpret_cast<unsigned char*>(sampBase + 0xA0890);
                unsigned char* pDL = reinterpret_cast<unsigned char*>(sampBase + 0xA0530);

                if (pR1 && !IsBadReadPtr(pR1, 5) && pR1[0] == 0x55 && pR1[1] == 0x8B && pR1[2] == 0xEC && pR1[3] == 0x83 && pR1[4] == 0xEC)
                {
                    s_Version = Version::R1;
                    s_Config = { "0.3.7-R1 (Signature)", 0x21A0F8, 0x21A10C, 0x9BD30, 0x9BC10, 0x3CD, 0x18, 0x2E, 0xFDE, 0x4, 0x6660, 0x6770, 0x6980 };
                }
                else if (pR3 && !IsBadReadPtr(pR3, 5) && pR3[0] == 0x55 && pR3[1] == 0x8B && pR3[2] == 0xEC && pR3[3] == 0x83 && pR3[4] == 0xEC)
                {
                    s_Version = Version::R3;
                    s_Config = { "0.3.7-R3 (Signature)", 0x26E8DC, 0x26E8F4, 0x9FFE0, 0x9FEC0, 0x3DE, 0x8, 0x4, 0xFB4, 0x2F1C, 0x6670, 0x6780, 0x6990 };
                }
                else if (pR4 && !IsBadReadPtr(pR4, 5) && pR4[0] == 0x55 && pR4[1] == 0x8B && pR4[2] == 0xEC && pR4[3] == 0x83 && pR4[4] == 0xEC)
                {
                    s_Version = Version::R4;
                    s_Config = { "0.3.7-R4 (Signature)", 0x26EA04, 0x26EA0C, 0xA0750, 0xA0630, 0x3DE, 0x8, 0x4, 0xFB4, 0x2F1C, 0x6670, 0x6780, 0x6990 };
                }
                else if (pR5 && !IsBadReadPtr(pR5, 5) && pR5[0] == 0x55 && pR5[1] == 0x8B && pR5[2] == 0xEC && pR5[3] == 0x83 && pR5[4] == 0xEC)
                {
                    s_Version = Version::R5;
                    s_Config = { "0.3.7-R5 (Signature)", 0x26EB94, 0x26EBAC, 0xA0890, 0xA0770, 0x3DE, 0x4, 0x4, 0xFB4, 0x2F1C, 0x6670, 0x6780, 0x6990 };
                }
                else if (pDL && !IsBadReadPtr(pDL, 5) && pDL[0] == 0x55 && pDL[1] == 0x8B && pDL[2] == 0xEC && pDL[3] == 0x83 && pDL[4] == 0xEC)
                {
                    s_Version = Version::DL;
                    s_Config = { "0.3.DL-1 (Signature)", 0x2ACA14, 0x2ACA24, 0xA0530, 0xA0410, 0x3DE, 0x8, 0x4, 0xFB4, 0x2F1C, 0x6670, 0x6780, 0x6990 };
                }
                else
                {
                    // Versão não comprovada: NUNCA assumir R3 cegamente!
                    s_Version = Version::Unknown;
                    s_Config = { "Unknown / Unsupported", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
                    Logger::Log("[SAMP] AVISO CRITICO: Versao do SA-MP nao reconhecida (EP=0x%X, TimeStamp=0x%X). Acesso a estruturas do SAMP bloqueado.",
                        entryPoint, timeDateStamp);
                }
            }

            // Localiza a funcao oficial de cor do jogador do SA-MP (GetColorForPlayer)
            if (sampBase && pNt)
            {
                __try
                {
                    DWORD textBase = sampBase + pNt->OptionalHeader.BaseOfCode;
                    DWORD textSize = pNt->OptionalHeader.SizeOfCode;
                    const unsigned char* pCode = reinterpret_cast<const unsigned char*>(textBase);
                    static const unsigned char pat[] = { 0x3D, 0xEC, 0x03, 0x00, 0x00, 0x75 };
                    for (DWORD offset = 4; offset + sizeof(pat) <= textSize; ++offset)
                    {
                        if (memcmp(pCode + offset, pat, sizeof(pat)) == 0)
                        {
                            if (pCode[offset - 4] == 0x8B && pCode[offset - 3] == 0x44 && pCode[offset - 2] == 0x24 && pCode[offset - 1] == 0x04)
                            {
                                s_fnGetColorForPlayer = reinterpret_cast<GetColorForPlayer_t>(const_cast<unsigned char*>(pCode + offset - 4));
                                Logger::Log("[SAMP] GetColorForPlayer located at 0x%p", s_fnGetColorForPlayer);
                                break;
                            }
                        }
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {}
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

        __try
        {
            return *reinterpret_cast<uintptr_t*>(sampBase + s_Config.infoOffset);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }

    uintptr_t GetPools()
    {
        uintptr_t pSAMP = GetSAMPInfo();
        if (!pSAMP) return 0;

        __try
        {
            return *reinterpret_cast<uintptr_t*>(pSAMP + s_Config.poolsOffset);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }

    uintptr_t GetPlayerPool()
    {
        uintptr_t pPools = GetPools();
        if (!pPools) return 0;

        __try
        {
            return *reinterpret_cast<uintptr_t*>(pPools + s_Config.playerPoolOffset);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }

    uint16_t GetLocalPlayerId()
    {
        uintptr_t pPlayerPool = GetPlayerPool();
        if (!pPlayerPool) return 0xFFFF;

        __try
        {
            return *reinterpret_cast<uint16_t*>(pPlayerPool + s_Config.localPlayerIdOffset);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0xFFFF;
        }
    }

    uintptr_t GetIsListedOffset()
    {
        return s_Config.isListedOffset;
    }

    int GetLargestPlayerId()
    {
        uintptr_t pPlayerPool = GetPlayerPool();
        if (!pPlayerPool) return 0;
        __try
        {
            int largest = *reinterpret_cast<int*>(pPlayerPool);
            if (largest >= 0 && largest < 1004)
                return largest;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return 1003;
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
            if (ppGta)
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

            // 1. Testa primeiro o offset primário determinístico por versão
            if (primaryOff != 0)
            {
                uintptr_t pCandidate = *reinterpret_cast<uintptr_t*>(pPlayerPool + primaryOff);
                if (pCandidate >= 0x10000 && pCandidate <= 0x7FFE0000)
                {
                    return pCandidate;
                }
            }

            // 2. Probing secundário nos offsets de pools conhecidos do SA-MP
            static const uintptr_t candidateOffsets[] = {
                0x22, 0x2F1C, 0x2F38, 0x2F3C, 0x2F20, 0x20, 0x24, 0x18, 0x2E
            };

            for (uintptr_t off : candidateOffsets)
            {
                uintptr_t pCandidate = *reinterpret_cast<uintptr_t*>(pPlayerPool + off);
                if (pCandidate >= 0x10000 && pCandidate <= 0x7FFE0000)
                {
                    return pCandidate;
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
                if (IsBadWritePtr(reinterpret_cast<void*>(pData), 68))
                    continue;

                if (hasPos)
                {
                    float* pPos = reinterpret_cast<float*>(pData + 6);
                    if (!IsBadReadPtr(pPos, 12))
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

    bool GetRemotePlayer(int index, RemotePlayerData& outData, uintptr_t pPlayerPool)
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

        if (!pPlayerPool)
            pPlayerPool = GetPlayerPool();
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
            if (pRemotePlayer < 0x10000 || pRemotePlayer > 0x7FFE0000)
                return false;

            outData.isValid = true;

            // 2.1 Lê cor e team do jogador remoto
            outData.color = GetPlayerColor(index);
            if (outData.color == 0)
            {
                uint32_t raw = *reinterpret_cast<uint32_t*>(pRemotePlayer + 0x28);
                if (raw == 0)
                    raw = *reinterpret_cast<uint32_t*>(pRemotePlayer + 0x24);
                if (raw != 0)
                {
                    if ((raw & 0xFF000000) != 0)
                        outData.color = raw | 0xFF000000;
                    else
                        outData.color = (raw >> 8) | 0xFF000000;
                }
            }
            outData.team = static_cast<int>(*reinterpret_cast<uint8_t*>(pRemotePlayer + 0x8));

            // 3. Lê o Nome do Jogador
            const char* pName = reinterpret_cast<const char*>(pRemotePlayer + 0xC);
            if (pName && pName[0] != '\0')
            {
                size_t nameRes = *reinterpret_cast<size_t*>(pRemotePlayer + 0x1C);
                if (nameRes >= 16)
                {
                    const char* pHeapName = *reinterpret_cast<const char**>(pRemotePlayer + 0xC);
                    if (pHeapName && pHeapName[0] != '\0' && uintptr_t(pHeapName) >= 0x10000 && uintptr_t(pHeapName) <= 0x7FFE0000)
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
            if (pPlayerData < 0x10000 || pPlayerData > 0x7FFE0000)
                return true;

            // 5. Obtém pSAMP_Actor (offset 0x0 de pPlayerData)
            uintptr_t pSAMP_Actor = *reinterpret_cast<uintptr_t*>(pPlayerData + 0x0);
            if (pSAMP_Actor < 0x10000 || pSAMP_Actor > 0x7FFE0000)
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

            if (pGtaPed && uintptr_t(pGtaPed) >= 0x10000 && uintptr_t(pGtaPed) <= 0x7FFE0000)
            {
                GTA::GetPedPosition(pGtaPed, outData.position);
                outData.health = GTA::GetPedHealth(pGtaPed);
                outData.armor  = GTA::GetPedArmor(pGtaPed);
            }
            else
            {
                float* pPos = reinterpret_cast<float*>(pPlayerData + (s_Version == Version::R1 ? 0x7B : 0x58));
                if (pPos && uintptr_t(pPos) >= 0x10000 && uintptr_t(pPos) <= 0x7FFE0000)
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

    uint32_t GetPlayerColor(int playerId)
    {
        if (playerId < 0 || playerId >= 1004) return 0;

        __try
        {
            if (s_fnGetColorForPlayer)
            {
                uint32_t rgba = s_fnGetColorForPlayer(playerId);
                if (rgba != 0)
                {
                    // Converte RGBA (0xRRGGBBAA) para ARGB (0xFFRRGGBB) exatamente como CRemotePlayer::GetPlayerColorAsARGB() do SA-MP
                    return (rgba >> 8) | 0xFF000000;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        // Fallback deterministico: le diretamente da tabela de cores em R1/R2
        uintptr_t sampBase = GetBaseAddress();
        if (sampBase && s_Config.infoOffset == 0x21A0F8)
        {
            __try
            {
                uint32_t* pTable = reinterpret_cast<uint32_t*>(sampBase + 0x216378);
                if (pTable)
                {
                    uint32_t rgba = pTable[playerId];
                    if (rgba != 0)
                    {
                        return (rgba >> 8) | 0xFF000000;
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        return 0;
    }

    uint32_t GetLocalPlayerColor()
    {
        uint16_t localId = GetLocalPlayerId();
        uint32_t col = GetPlayerColor(localId);
        if (col != 0) return col;

        uintptr_t pPlayerPool = GetPlayerPool();
        if (!pPlayerPool) return 0;

        __try
        {
            if (s_Version == Version::R1 || s_Version == Version::R2)
            {
                uint32_t col = *reinterpret_cast<uint32_t*>(pPlayerPool + 0x8);
                if (col != 0) return col;
            }
            else // R3, R4, R5, DL
            {
                // Offsets onde m_dwLocalPlayerColor reside em CPlayerPool (0.3.7-R3 / R4 / DL)
                static const uintptr_t colorOffsets[] = { 0x2F44, 0x2F40, 0x2F3C, 0x2F48, 0x2F18, 0x2F20, 0x8 };
                for (uintptr_t off : colorOffsets)
                {
                    uint32_t val = *reinterpret_cast<uint32_t*>(pPlayerPool + off);
                    // Checa se parece com uma cor valida do SA-MP (ARGB com alpha 0xFF ou RGBA com alpha 0xFF)
                    // e que nao seja um ponteiro de heap ou ID baixo
                    if (val != 0 && val > 0x10000 && ((val & 0xFF000000) == 0xFF000000 || (val & 0x000000FF) == 0xFF))
                    {
                        return val;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return 0;
    }

    static inline uint32_t NormalizeColorRGB(uint32_t col)
    {
        if (col == 0) return 0;
        // Se tiver canal alfa no byte superior (0xAARRGGBB - D3DCOLOR / SA-MP standard)
        if ((col & 0xFF000000) != 0)
        {
            return (col & 0x00FFFFFF);
        }
        // Se tiver canal alfa no byte inferior (0xRRGGBBAA - Pawn standard)
        return ((col >> 8) & 0x00FFFFFF);
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

        // 1. Checagem por Team do Servidor (SetPlayerTeam)
        uint8_t localTeam = 255;
        uintptr_t pLocal = GetLocalPlayer();
        if (pLocal && pLocal >= 0x10000 && pLocal <= 0x7FFE0000)
        {
            localTeam = *reinterpret_cast<uint8_t*>(pLocal + 0x8);
        }

        if (localTeam != 255 && rp.team != 255 && rp.team != -1)
        {
            if (rp.team == static_cast<int>(localTeam))
                return true;
        }

        // 2. Checagem por Cor da Organizacao/Faccao (SetPlayerColor)
        uint32_t localColor = GetLocalPlayerColor();
        uint32_t localRGB = NormalizeColorRGB(localColor);
        uint32_t remoteRGB = NormalizeColorRGB(rp.color);

        // Se a cor do player remoto nao estiver em rp.color, tenta ler m_dwMarkerColor (+0x24)
        if (remoteRGB == 0)
        {
            uintptr_t pPlayerPool = GetPlayerPool();
            if (pPlayerPool)
            {
                uintptr_t pRemotePlayer = *reinterpret_cast<uintptr_t*>(pPlayerPool + s_Config.remotePlayerOffset + index * 4);
                if (pRemotePlayer >= 0x10000 && pRemotePlayer <= 0x7FFE0000)
                {
                    uint32_t markerCol = *reinterpret_cast<uint32_t*>(pRemotePlayer + 0x24);
                    remoteRGB = NormalizeColorRGB(markerCol);
                }
            }
        }

        // Se ambos tem cor de organizacao valida definida
        if (localRGB != 0 && remoteRGB != 0)
        {
            if (localRGB == remoteRGB)
            {
                return true;
            }
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
    static void* s_pRakClient = nullptr;

    void* GetRakClient()
    {
        return s_pRakClient;
    }

    struct SimpleBitStream
    {
        int numberOfBitsUsed;
        int numberOfBitsAllocated;
        int readOffset;
        unsigned char* data;
        bool copyData;
        unsigned char stackData[256];
    };

    bool SendRawPacket(const unsigned char* data, int length, int priority, int reliability, char orderingChannel)
    {
        if (!s_pRakClient || !data || length <= 0)
            return false;

        if (s_OriginalSendBitStream)
        {
            __try
            {
                SimpleBitStream bs = {};
                bs.numberOfBitsUsed = length * 8;
                bs.numberOfBitsAllocated = length * 8;
                bs.readOffset = 0;
                bs.data = const_cast<unsigned char*>(data);
                bs.copyData = false;
                return s_OriginalSendBitStream(s_pRakClient, &bs, priority, reliability, orderingChannel);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }

        if (s_OriginalSendData)
        {
            __try
            {
                return s_OriginalSendData(s_pRakClient, reinterpret_cast<const char*>(data), length, priority, reliability, orderingChannel);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }
        }

        return false;
    }

    static bool __fastcall Hooked_SendBitStream(void* pThis, void* edx, void* pBitStream, int priority, int reliability, char orderingChannel)
    {
        Main::CallbackGuard guard;

        if (pThis)
        {
            s_pRakClient = pThis;
        }

        if (!s_OriginalSendBitStream)
        {
            return false;
        }

        if (!guard.IsActive())
        {
            return s_OriginalSendBitStream(pThis, pBitStream, priority, reliability, orderingChannel);
        }

        if (pBitStream && !IsBadReadPtr(pBitStream, 16))
        {
            int numberOfBitsUsed = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(pBitStream) + 0);
            int byteCount = (numberOfBitsUsed + 7) / 8;
            unsigned char* data = *reinterpret_cast<unsigned char**>(reinterpret_cast<uintptr_t>(pBitStream) + 12);
            if (data && byteCount >= 1)
            {
                unsigned char packetId = data[0];

                // Interceptação de comandos do chat (/tapa, /slap, /derrubar)
                if (g_MenuState.playerSlap.chatCommands && (packetId == 32 || packetId == 50 || packetId == 101))
                {
                    if (PlayerSlap::ProcessCommandPacket(data, byteCount))
                    {
                        return true; // Suprime envio para o servidor
                    }
                }

                // Diagnóstico durante disparo (LMB pressionado) ou para pacotes de sync/combate
                bool isShooting = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                if (packetId == 206)
                {
                    Logger::Log("[SAMP][DIAG][NET] Hooked_SendBitStream: ID_BULLET_SYNC (206) interceptado! byteCount=%d (bits=%d)",
                        byteCount, numberOfBitsUsed);
                    SilentAim::MutateBulletSyncPacket(data, byteCount);
                }
                else if (packetId == 32 && byteCount >= 14)
                {
                    AntiAim::ProcessDamageRPC(data, byteCount, numberOfBitsUsed);
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
                    static uint64_t s_last207Log = 0;
                    uint64_t now = GetTickCount64();
                    if (now - s_last207Log >= 3000)
                    {
                        Logger::Log("[SAMP][NET] Packet 207 interceptado com sucesso! byteCount=%d", byteCount);
                        s_last207Log = now;
                    }
                    AntiAim::MutateOnFootPacket(data, byteCount);
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

        if (pThis)
        {
            s_pRakClient = pThis;
        }

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

            // Interceptação de comandos do chat (/tapa, /slap, /derrubar)
            if (g_MenuState.playerSlap.chatCommands && (packetId == 32 || packetId == 50 || packetId == 101))
            {
                if (PlayerSlap::ProcessCommandPacket(reinterpret_cast<const unsigned char*>(data), length))
                {
                    return true; // Suprime envio para o servidor
                }
            }

            bool isShooting = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            if (packetId == 206)
            {
                Logger::Log("[SAMP][DIAG][NET] Hooked_SendData: ID_BULLET_SYNC (206) interceptado! length=%d", length);
                if (!IsBadWritePtr(const_cast<char*>(data), length))
                {
                    SilentAim::MutateBulletSyncPacket(reinterpret_cast<unsigned char*>(const_cast<char*>(data)), length);
                }
            }
            else if (packetId == 32 && length >= 14)
            {
                if (!IsBadWritePtr(const_cast<char*>(data), length))
                {
                    AntiAim::ProcessDamageRPC(reinterpret_cast<unsigned char*>(const_cast<char*>(data)), length, length * 8);
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
                if (!IsBadWritePtr(const_cast<char*>(data), length))
                {
                    AntiAim::MutateOnFootPacket(reinterpret_cast<unsigned char*>(const_cast<char*>(data)), length);
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

    static bool IsValidCodePointer(uintptr_t ptr)
    {
        if (ptr < 0x10000 || ptr > 0x7FFE0000)
            return false;

        MEMORY_BASIC_INFORMATION mbi = {};
        if (VirtualQuery(reinterpret_cast<void*>(ptr), &mbi, sizeof(mbi)) == 0)
            return false;

        if (mbi.State != MEM_COMMIT)
            return false;

        DWORD prot = (mbi.Protect & 0xFF);
        return (prot == PAGE_EXECUTE || prot == PAGE_EXECUTE_READ ||
                prot == PAGE_EXECUTE_READWRITE || prot == PAGE_EXECUTE_WRITECOPY);
    }

    typedef void(__thiscall* SendTakeDamage_t)(void* pThis, int nId, float fDamage, int nWeapon, int nBodyPart);
    static SendTakeDamage_t s_OriginalSendTakeDamage = nullptr;
    static unsigned char s_TrampolineSendTakeDamage[32] = { 0 };
    static unsigned char s_OriginalTakeDamageBytes[7] = { 0 };
    static uintptr_t s_TakeDamageTarget = 0;
    static bool s_TakeDamageHookInstalled = false;

    static void __fastcall Hooked_SendTakeDamage(void* pThis, void* edx, int nId, float fDamage, int nWeapon, int nBodyPart)
    {
        Main::CallbackGuard guard;

        if (!s_OriginalSendTakeDamage)
        {
            return;
        }

        if (!guard.IsActive())
        {
            s_OriginalSendTakeDamage(pThis, nId, fDamage, nWeapon, nBodyPart);
            return;
        }

        if (g_MenuState.player.antiHS)
        {
            // 1. Interceptação de Headshot (Bone 9 -> Bone 3)
            if (nBodyPart == 9)
            {
                nBodyPart = 3; // Converte Cabeça para Peito
                if (fDamage > g_MenuState.player.antiHSDamageCap)
                {
                    fDamage = g_MenuState.player.antiHSDamageCap;
                }
                Logger::Log("[SAMP][ANTI-HS] SendTakeDamage interceptado! Bone 9 (Cabeca) -> 3 (Peito). Dano regulado=%.1f", fDamage);
            }
            else if (fDamage > g_MenuState.player.antiHSDamageCap)
            {
                fDamage = g_MenuState.player.antiHSDamageCap;
            }

            // 2. Protege imediatamente o Ped local contra morte repentina por HS
            void* pLocalPed = RuntimeState::GetLocalPed();
            if (!pLocalPed && pThis)
            {
                pLocalPed = *reinterpret_cast<void**>(pThis);
            }

            if (pLocalPed && !IsBadReadPtr(pLocalPed, 0x550))
            {
                uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pLocalPed);
                float* pHealth = reinterpret_cast<float*>(pedAddr + 0x540);
                uint32_t* pPedState = reinterpret_cast<uint32_t*>(pedAddr + 0x530);

                if (pHealth && !IsBadReadPtr(pHealth, sizeof(float)))
                {
                    if (*pHealth <= 0.0f || (pPedState && (*pPedState == 0x36 || *pPedState == 0x37)))
                    {
                        *pHealth = 15.0f;
                        if (pPedState && (*pPedState == 0x36 || *pPedState == 0x37))
                        {
                            *pPedState = 1; // PED_STATE_IDLE
                        }
                        Logger::Log("[SAMP][ANTI-HS] Morte local impedida no SendTakeDamage! Vida restaurada para 15.0 HP");
                    }
                }
            }
        }

        s_OriginalSendTakeDamage(pThis, nId, fDamage, nWeapon, nBodyPart);
    }

    bool EnsureSendTakeDamageHook()
    {
        if (Main::IsShuttingDown()) return false;
        if (s_TakeDamageHookInstalled) return true;

        uintptr_t sampBase = GetBaseAddress();
        if (!sampBase || s_Config.sendTakeDamageOffset == 0) return false;

        uintptr_t target = sampBase + s_Config.sendTakeDamageOffset;
        if (IsBadReadPtr(reinterpret_cast<void*>(target), 7)) return false;

        // Salva os 7 bytes originais
        memcpy(s_OriginalTakeDamageBytes, reinterpret_cast<void*>(target), 7);
        s_TakeDamageTarget = target;

        // Prepara o trampoline com protecao PAGE_EXECUTE_READWRITE
        DWORD oldProtect = 0;
        if (!VirtualProtect(s_TrampolineSendTakeDamage, sizeof(s_TrampolineSendTakeDamage), PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            return false;
        }

        // Copia os 7 bytes originais para o trampoline
        memcpy(s_TrampolineSendTakeDamage, reinterpret_cast<void*>(target), 7);

        // Adiciona JMP de volta para target + 7
        s_TrampolineSendTakeDamage[7] = 0xE9;
        uintptr_t trampJumpFrom = reinterpret_cast<uintptr_t>(&s_TrampolineSendTakeDamage[7]);
        uintptr_t trampJumpTo = target + 7;
        *reinterpret_cast<int32_t*>(&s_TrampolineSendTakeDamage[8]) = static_cast<int32_t>(trampJumpTo - (trampJumpFrom + 5));
        s_OriginalSendTakeDamage = reinterpret_cast<SendTakeDamage_t>(static_cast<void*>(s_TrampolineSendTakeDamage));

        // Instala o detour hook no target (E9 <rel32> 90 90)
        DWORD targetOldProtect = 0;
        if (VirtualProtect(reinterpret_cast<void*>(target), 7, PAGE_EXECUTE_READWRITE, &targetOldProtect))
        {
            unsigned char* pTargetBytes = reinterpret_cast<unsigned char*>(target);
            pTargetBytes[0] = 0xE9;
            uintptr_t hookAddr = reinterpret_cast<uintptr_t>(&Hooked_SendTakeDamage);
            *reinterpret_cast<int32_t*>(&pTargetBytes[1]) = static_cast<int32_t>(hookAddr - (target + 5));
            pTargetBytes[5] = 0x90; // NOP
            pTargetBytes[6] = 0x90; // NOP

            VirtualProtect(reinterpret_cast<void*>(target), 7, targetOldProtect, &targetOldProtect);
            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(target), 7);

            s_TakeDamageHookInstalled = true;
            Logger::Log("[SAMP][HOOK] CLocalPlayer::SendTakeDamage detour hook instalado com sucesso em 0x%p (offset 0x%X)",
                reinterpret_cast<void*>(target), s_Config.sendTakeDamageOffset);
            return true;
        }

        return false;
    }

    bool IsSendTakeDamageHooked()
    {
        return s_TakeDamageHookInstalled;
    }

    void RestoreSendTakeDamageHook()
    {
        if (!s_TakeDamageHookInstalled || !s_TakeDamageTarget) return;

        DWORD oldProtect = 0;
        if (VirtualProtect(reinterpret_cast<void*>(s_TakeDamageTarget), 7, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            memcpy(reinterpret_cast<void*>(s_TakeDamageTarget), s_OriginalTakeDamageBytes, 7);
            VirtualProtect(reinterpret_cast<void*>(s_TakeDamageTarget), 7, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(s_TakeDamageTarget), 7);
            Logger::Log("[SAMP][UNHOOK] CLocalPlayer::SendTakeDamage detour hook restaurado com sucesso.");
        }
        s_TakeDamageHookInstalled = false;
        s_TakeDamageTarget = 0;
        s_OriginalSendTakeDamage = nullptr;
    }

    void SendGiveDamage(int targetId, float damage, int weaponId, int bodyPart)
    {
        if (targetId < 0 || targetId >= 1004) return;
        DetectVersion();
        uintptr_t sampBase = GetBaseAddress();
        if (!sampBase || s_Config.sendGiveDamageOffset == 0) return;

        uintptr_t pLocal = GetLocalPlayer();
        if (!pLocal) return;

        __try
        {
            typedef void(__thiscall* SendGiveDamage_t)(void* pThis, int nId, float fDamage, int nWeapon, int nBodyPart);
            auto fnSendGive = reinterpret_cast<SendGiveDamage_t>(sampBase + s_Config.sendGiveDamageOffset);
            if (fnSendGive && !IsBadReadPtr(reinterpret_cast<void*>(fnSendGive), 4))
            {
                fnSendGive(reinterpret_cast<void*>(pLocal), targetId, damage, weaponId, bodyPart);
                Logger::Log("[SAMP][GIVE_DAMAGE] SendGiveDamage transmitido: alvo=#%d dano=%.2f arma=%d osso=%d",
                    targetId, damage, weaponId, bodyPart);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            Logger::Log("[SAMP][GIVE_DAMAGE] Excecao em SendGiveDamage");
        }
    }

#pragma pack(push, 1)
    struct stBulletSyncPacket
    {
        uint8_t packetId;       // 206 (ID_BULLET_SYNC)
        uint8_t byteType;       // 1 = BULLET_HIT_TYPE_PLAYER
        uint16_t sTargetID;     // target player ID
        float fOrigin[3];       // origin (X, Y, Z)
        float fTarget[3];       // target (X, Y, Z)
        float fCenter[3];       // center offset relative to ped
        uint8_t byteWeaponID;   // weapon ID
    };
#pragma pack(pop)
    static_assert(sizeof(stBulletSyncPacket) == 41, "stBulletSyncPacket must be 41 bytes");

    bool SendBulletData(uint16_t targetId, const float origin[3], const float target[3], const float center[3], uint8_t weaponId, uint8_t hitType)
    {
        if (targetId >= 1004) return false;
        DetectVersion();

        stBulletSyncPacket pkt = {};
        pkt.packetId = 206;
        pkt.byteType = hitType;
        pkt.sTargetID = targetId;
        if (origin)
        {
            pkt.fOrigin[0] = origin[0];
            pkt.fOrigin[1] = origin[1];
            pkt.fOrigin[2] = origin[2];
        }
        if (target)
        {
            pkt.fTarget[0] = target[0];
            pkt.fTarget[1] = target[1];
            pkt.fTarget[2] = target[2];
        }
        if (center)
        {
            pkt.fCenter[0] = center[0];
            pkt.fCenter[1] = center[1];
            pkt.fCenter[2] = center[2];
        }
        pkt.byteWeaponID = weaponId;

        bool invokedInternal = false;
        uintptr_t sampBase = GetBaseAddress();
        if (sampBase && s_Config.sendBulletDataOffset != 0)
        {
            uintptr_t pLocal = GetLocalPlayer();
            if (pLocal)
            {
                __try
                {
                    // CLocalPlayer::SendBulletData(stBulletData* pData)
                    // stBulletData é a porção de 40 bytes (sem packetId)
                    typedef void(__thiscall* SendBulletData_t)(void* pThis, void* pBulletData);
                    auto fnSendBullet = reinterpret_cast<SendBulletData_t>(sampBase + s_Config.sendBulletDataOffset);
                    if (fnSendBullet && !IsBadReadPtr(reinterpret_cast<void*>(fnSendBullet), 4))
                    {
                        fnSendBullet(reinterpret_cast<void*>(pLocal), &pkt.byteType);
                        invokedInternal = true;
                        Logger::Log("[SAMP][BULLET_SYNC] CLocalPlayer::SendBulletData nativo invocado: alvo=#%d arma=%d origin=(%.1f, %.1f, %.1f)",
                            targetId, weaponId, pkt.fOrigin[0], pkt.fOrigin[1], pkt.fOrigin[2]);
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    Logger::Log("[SAMP][BULLET_SYNC] Excecao em CLocalPlayer::SendBulletData");
                }
            }
        }

        // Transmissão direta via SendRawPacket caso CLocalPlayer::SendBulletData não esteja disponível
        bool rawOk = false;
        if (!invokedInternal)
        {
            rawOk = SendRawPacket(reinterpret_cast<const unsigned char*>(&pkt), sizeof(pkt), 1, 7, 0);
            Logger::Log("[SAMP][BULLET_SYNC] SendRawPacket (206) transmitido: alvo=#%d arma=%d origin=(%.1f, %.1f, %.1f) status=%s",
                targetId, weaponId, pkt.fOrigin[0], pkt.fOrigin[1], pkt.fOrigin[2], rawOk ? "OK" : "FALHA");
        }

        return (invokedInternal || rawOk);
    }

    bool EnsureRakHook()
    {
        if (Main::IsShuttingDown()) return false;

        // Garante também o hook de interceptação de dano do SAMP
        EnsureSendTakeDamageHook();

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
                directOff, 0x3C9, 0x3DA, 0x3CD, 0x3DE, 0x3C8, 0x3D8, 0x3D6, 0x3DC, 0x3E0, 0x3E2, 0x2C
            };

            for (uintptr_t off : candidateOffsets)
            {
                if (IsBadReadPtr(reinterpret_cast<void*>(sampInfo + off), sizeof(void*)))
                    continue;

                uintptr_t pCandidate = *reinterpret_cast<uintptr_t*>(sampInfo + off);
                if (pCandidate < 0x10000 || pCandidate > 0x7FFE0000)
                    continue;

                // Mode 1: pCandidate e um ponteiro para o objeto RakClient (cujo 1o DWORD e a VTable)
                void** vtable = *reinterpret_cast<void***>(pCandidate);
                if (vtable && !IsBadReadPtr(vtable, sizeof(void*) * 10))
                {
                    uintptr_t fn6 = reinterpret_cast<uintptr_t>(vtable[6]);
                    uintptr_t fn7 = reinterpret_cast<uintptr_t>(vtable[7]);
                    if (IsValidCodePointer(fn6) && IsValidCodePointer(fn7))
                    {
                        DWORD oldProtect = 0;
                        if (VirtualProtect(&vtable[6], sizeof(void*) * 2, PAGE_EXECUTE_READWRITE, &oldProtect))
                        {
                            s_OriginalSendBitStream = reinterpret_cast<SendBitStream_t>(vtable[6]);
                            s_OriginalSendData = reinterpret_cast<SendData_t>(vtable[7]);

                            vtable[6] = reinterpret_cast<void*>(&Hooked_SendBitStream);
                            vtable[7] = reinterpret_cast<void*>(&Hooked_SendData);

                            VirtualProtect(&vtable[6], sizeof(void*) * 2, oldProtect, &oldProtect);

                            s_HookedVTable = vtable;
                            s_HookedOffset = off;
                            s_RakHookInstalled = true;
                            Logger::Log("[SAMP][DIAG][HOOK_SUCCESS] RakClient VMT hook instalado com sucesso! Offset=0x%X, VTable=%p, origSendBitStream=%p, origSendData=%p",
                                off, vtable, s_OriginalSendBitStream, s_OriginalSendData);
                            return true;
                        }
                        else
                        {
                            Logger::Log("[SAMP][DIAG][HOOK_FAIL] VirtualProtect falhou ao alterar protecao da VTable no offset 0x%X", off);
                        }
                    }
                }

                // Mode 2: pCandidate ja e o ponteiro direto da VTable (objeto embutido)
                void** directVTable = reinterpret_cast<void**>(pCandidate);
                if (directVTable && !IsBadReadPtr(directVTable, sizeof(void*) * 10))
                {
                    uintptr_t fn6 = reinterpret_cast<uintptr_t>(directVTable[6]);
                    uintptr_t fn7 = reinterpret_cast<uintptr_t>(directVTable[7]);
                    if (IsValidCodePointer(fn6) && IsValidCodePointer(fn7))
                    {
                        DWORD oldProtect = 0;
                        if (VirtualProtect(&directVTable[6], sizeof(void*) * 2, PAGE_EXECUTE_READWRITE, &oldProtect))
                        {
                            s_OriginalSendBitStream = reinterpret_cast<SendBitStream_t>(directVTable[6]);
                            s_OriginalSendData = reinterpret_cast<SendData_t>(directVTable[7]);

                            directVTable[6] = reinterpret_cast<void*>(&Hooked_SendBitStream);
                            directVTable[7] = reinterpret_cast<void*>(&Hooked_SendData);

                            VirtualProtect(&directVTable[6], sizeof(void*) * 2, oldProtect, &oldProtect);

                            s_HookedVTable = directVTable;
                            s_HookedOffset = off;
                            s_RakHookInstalled = true;
                            Logger::Log("[SAMP][DIAG][HOOK_SUCCESS] RakClient direct VMT hook instalado! Offset=0x%X, VTable=%p, origSendBitStream=%p, origSendData=%p",
                                off, directVTable, s_OriginalSendBitStream, s_OriginalSendData);
                            return true;
                        }
                    }
                }
            }

            // Fallback: varre a memoria de sampInfo byte a byte (0x20..0x450) para suportar packing desalinhado
            for (uintptr_t off = 0x20; off <= 0x450; off++)
            {
                if (IsBadReadPtr(reinterpret_cast<void*>(sampInfo + off), sizeof(void*)))
                    continue;

                uintptr_t pCandidate = *reinterpret_cast<uintptr_t*>(sampInfo + off);
                if (pCandidate < 0x10000 || pCandidate > 0x7FFE0000)
                    continue;

                void** vtable = *reinterpret_cast<void***>(pCandidate);
                if (vtable && !IsBadReadPtr(vtable, sizeof(void*) * 10))
                {
                    uintptr_t fn6 = reinterpret_cast<uintptr_t>(vtable[6]);
                    uintptr_t fn7 = reinterpret_cast<uintptr_t>(vtable[7]);
                    if (IsValidCodePointer(fn6) && IsValidCodePointer(fn7))
                    {
                        DWORD oldProtect = 0;
                        if (VirtualProtect(&vtable[6], sizeof(void*) * 2, PAGE_EXECUTE_READWRITE, &oldProtect))
                        {
                            s_OriginalSendBitStream = reinterpret_cast<SendBitStream_t>(vtable[6]);
                            s_OriginalSendData = reinterpret_cast<SendData_t>(vtable[7]);

                            vtable[6] = reinterpret_cast<void*>(&Hooked_SendBitStream);
                            vtable[7] = reinterpret_cast<void*>(&Hooked_SendData);

                            VirtualProtect(&vtable[6], sizeof(void*) * 2, oldProtect, &oldProtect);

                            s_HookedVTable = vtable;
                            s_HookedOffset = off;
                            s_RakHookInstalled = true;
                            Logger::Log("[SAMP][DIAG][HOOK_SUCCESS] RakClient VMT hook instalado via fallback scan! Offset=0x%X, VTable=%p",
                                off, vtable);
                            return true;
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
        // Restaura sempre o hook de SendTakeDamage
        RestoreSendTakeDamageHook();

        if (!s_RakHookInstalled)
        {
            s_TeardownStatus = TeardownStatus::NotHooked;
            return s_TeardownStatus;
        }

        bool restored = false;

        // 1. Tenta restaurar diretamente pela VTable cacheada
        if (s_HookedVTable && !IsBadWritePtr(s_HookedVTable, sizeof(void*) * 10))
        {
            __try
            {
                bool isOurHook = (s_HookedVTable[6] == reinterpret_cast<void*>(&Hooked_SendBitStream)) ||
                                 (s_HookedVTable[7] == reinterpret_cast<void*>(&Hooked_SendData));
                if (isOurHook && s_OriginalSendBitStream && s_OriginalSendData)
                {
                    DWORD oldProtect = 0;
                    if (VirtualProtect(&s_HookedVTable[6], sizeof(void*) * 2, PAGE_EXECUTE_READWRITE, &oldProtect))
                    {
                        s_HookedVTable[6] = reinterpret_cast<void*>(s_OriginalSendBitStream);
                        s_HookedVTable[7] = reinterpret_cast<void*>(s_OriginalSendData);
                        VirtualProtect(&s_HookedVTable[6], sizeof(void*) * 2, oldProtect, &oldProtect);
                        restored = true;
                        Logger::Log("[SAMP][DIAG][UNHOOK] RakClient VMT hook restaurado via cached VTable (offset 0x%X)", s_HookedOffset);
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        // 2. Fallback: Se nao restaurou pela vtable cacheada, varre os offsets candidatos
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
                        if (IsBadReadPtr(reinterpret_cast<void*>(sampInfo + off), sizeof(void*)))
                            continue;

                        uintptr_t pCandidate = *reinterpret_cast<uintptr_t*>(sampInfo + off);
                        if (pCandidate <= 0x10000 || IsBadReadPtr(reinterpret_cast<void*>(pCandidate), sizeof(void*)))
                            continue;

                        void** vtable = *reinterpret_cast<void***>(pCandidate);
                        if (!vtable || IsBadReadPtr(vtable, sizeof(void*) * 10))
                            continue;

                        bool isOurHook = (vtable[6] == reinterpret_cast<void*>(&Hooked_SendBitStream)) ||
                                         (vtable[7] == reinterpret_cast<void*>(&Hooked_SendData));
                        if (isOurHook)
                        {
                            DWORD oldProtect = 0;
                            if (VirtualProtect(&vtable[6], sizeof(void*) * 2, PAGE_EXECUTE_READWRITE, &oldProtect))
                            {
                                vtable[6] = reinterpret_cast<void*>(s_OriginalSendBitStream);
                                vtable[7] = reinterpret_cast<void*>(s_OriginalSendData);
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
