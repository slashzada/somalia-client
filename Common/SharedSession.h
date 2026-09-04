#pragma once
#include <windows.h>
#include <sddl.h>
#include <string>
#include <cstring>
#include <vector>

#pragma comment(lib, "advapi32.lib")

namespace SharedSession
{
    static const char* MAPPING_NAME = "Local\\Somalia_Runtime_Session_Mem";
    static const char* MAPPING_NAME_FALLBACK = "Somalia_Runtime_Session_Mem";
    constexpr size_t SESSION_BUFFER_SIZE = 256;
    constexpr size_t MAX_INPUT_SIZE = 127;

    struct SessionData
    {
        uint32_t magic;      // 0x534F4D53 ('SOMS')
        uint32_t version;    // 1
        char sessionId[128];
    };

    constexpr uint32_t MAGIC_HEADER = 0x534F4D53;

    // Construtor RAII de descritor de segurança com DACL restrita (SDDL)
    // Permissões concedidas:
    // - Creator Owner (CO): GA (Generic All / Controle Total para escrita e encerramento pelo Loader)
    // - Usuário Atual (User SID): A;;0x0004 (SECTION_MAP_READ / Somente Leitura)
    // - NENHUM processo terceiro/não autorizado recebe permissão de escrita (SECTION_MAP_WRITE bloqueado pelo Kernel)
    struct SecurityAttributesGuard
    {
        SECURITY_ATTRIBUTES sa;
        PSECURITY_DESCRIPTOR pSD;

        SecurityAttributesGuard() : pSD(nullptr)
        {
            ZeroMemory(&sa, sizeof(sa));
            sa.nLength = sizeof(SECURITY_ATTRIBUTES);
            sa.bInheritHandle = FALSE;

            std::string userSidStr = "";
            HANDLE hToken = NULL;
            if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            {
                DWORD dwSize = 0;
                GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && dwSize > 0)
                {
                    std::vector<BYTE> buffer(dwSize);
                    PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>(buffer.data());
                    if (GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize))
                    {
                        LPSTR pStrSid = NULL;
                        if (ConvertSidToStringSidA(pTokenUser->User.Sid, &pStrSid))
                        {
                            userSidStr = pStrSid;
                            LocalFree(pStrSid);
                        }
                    }
                }
                CloseHandle(hToken);
            }

            // D:P = DACL protegida (sem herança de regras permissivas do container)
            // (A;;GA;;;CO) = Allow Generic All para Creator Owner
            // (A;;0x0004;;;[UserSID]) = SECTION_MAP_READ estritamente para o SID do usuário
            std::string sddl;
            if (!userSidStr.empty())
            {
                sddl = "D:P(A;;GA;;;CO)(A;;0x0004;;;" + userSidStr + ")";
            }
            else
            {
                // Fallback seguro: SECTION_MAP_READ para Interactive Users (IU)
                sddl = "D:P(A;;GA;;;CO)(A;;0x0004;;;IU)";
            }

            if (ConvertStringSecurityDescriptorToSecurityDescriptorA(
                sddl.c_str(), SDDL_REVISION_1, &pSD, NULL))
            {
                sa.lpSecurityDescriptor = pSD;
            }
        }

        ~SecurityAttributesGuard()
        {
            if (pSD)
            {
                LocalFree(pSD);
                pSD = nullptr;
            }
        }
    };

    inline HANDLE OpenOrCreateMapping(bool create, DWORD desiredAccess, DWORD targetPid = 0)
    {
        std::string baseName = MAPPING_NAME;
        std::string fallbackName = MAPPING_NAME_FALLBACK;

        if (targetPid != 0)
        {
            baseName = "Local\\SomaliaSession_" + std::to_string(targetPid);
            fallbackName = "SomaliaSession_" + std::to_string(targetPid);
        }

        HANDLE hMap = nullptr;
        if (create)
        {
            SecurityAttributesGuard secGuard;
            LPSECURITY_ATTRIBUTES pSA = secGuard.sa.lpSecurityDescriptor ? &secGuard.sa : NULL;

            hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, pSA, PAGE_READWRITE, 0, sizeof(SessionData), baseName.c_str());
            if (!hMap)
            {
                hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, pSA, PAGE_READWRITE, 0, sizeof(SessionData), fallbackName.c_str());
            }
        }
        else
        {
            if (targetPid != 0)
            {
                hMap = OpenFileMappingA(desiredAccess, FALSE, baseName.c_str());
                if (!hMap)
                {
                    hMap = OpenFileMappingA(desiredAccess, FALSE, fallbackName.c_str());
                }
            }

            // Se estivermos dentro do GTA (Native), tenta o PID do processo corrente
            if (!hMap)
            {
                DWORD currentPid = GetCurrentProcessId();
                std::string currentPidName = "Local\\SomaliaSession_" + std::to_string(currentPid);
                std::string currentPidFallback = "SomaliaSession_" + std::to_string(currentPid);
                hMap = OpenFileMappingA(desiredAccess, FALSE, currentPidName.c_str());
                if (!hMap)
                {
                    hMap = OpenFileMappingA(desiredAccess, FALSE, currentPidFallback.c_str());
                }
            }

            // Fallback para o nome global padrão
            if (!hMap)
            {
                hMap = OpenFileMappingA(desiredAccess, FALSE, MAPPING_NAME);
                if (!hMap)
                {
                    hMap = OpenFileMappingA(desiredAccess, FALSE, MAPPING_NAME_FALLBACK);
                }
            }
        }
        return hMap;
    }

    inline bool SafeWriteSessionData(void* pBuf, const char* sessionId)
    {
        __try
        {
            SessionData* data = reinterpret_cast<SessionData*>(pBuf);
            SecureZeroMemory(data, sizeof(SessionData));
            data->magic = MAGIC_HEADER;
            data->version = 1;
            strncpy_s(data->sessionId, sessionId, sizeof(data->sessionId) - 1);
            data->sessionId[sizeof(data->sessionId) - 1] = '\0';
            FlushViewOfFile(pBuf, sizeof(SessionData));
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    inline void SafeClearSessionData(void* pBuf, size_t size)
    {
        __try
        {
            SecureZeroMemory(pBuf, size);
            FlushViewOfFile(pBuf, size);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    inline bool SafeReadSessionData(const void* pBuf, char* outSessionId, size_t maxOut)
    {
        __try
        {
            const SessionData* data = reinterpret_cast<const SessionData*>(pBuf);
            if (data->magic != MAGIC_HEADER || data->version != 1)
            {
                return false;
            }

            size_t maxLen = sizeof(data->sessionId);
            if (maxLen > maxOut) maxLen = maxOut;

            size_t len = 0;
            while (len < maxLen && data->sessionId[len] != '\0')
            {
                char c = data->sessionId[len];
                if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_'))
                {
                    return false;
                }
                outSessionId[len] = c;
                len++;
            }

            if (len == 0 || len >= maxOut)
            {
                return false;
            }
            outSessionId[len] = '\0';
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    inline bool PublishSession(const std::string& sessionId, HANDLE& outHandle, DWORD targetPid = 0)
    {
        if (outHandle && outHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(outHandle);
            outHandle = nullptr;
        }

        if (sessionId.empty() || sessionId.length() >= sizeof(SessionData::sessionId)) return false;

        outHandle = OpenOrCreateMapping(true, FILE_MAP_ALL_ACCESS, targetPid);
        if (!outHandle) return false;

        void* pBuf = MapViewOfFile(outHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SessionData));
        if (!pBuf)
        {
            CloseHandle(outHandle);
            outHandle = nullptr;
            return false;
        }

        if (!SafeWriteSessionData(pBuf, sessionId.c_str()))
        {
            UnmapViewOfFile(pBuf);
            CloseHandle(outHandle);
            outHandle = nullptr;
            return false;
        }

        UnmapViewOfFile(pBuf);
        return true;
    }

    inline void RevokeSession(HANDLE& hHandle)
    {
        if (hHandle && hHandle != INVALID_HANDLE_VALUE)
        {
            void* pBuf = MapViewOfFile(hHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SessionData));
            if (pBuf)
            {
                SafeClearSessionData(pBuf, sizeof(SessionData));
                UnmapViewOfFile(pBuf);
            }
            CloseHandle(hHandle);
            hHandle = nullptr;
        }
    }

    inline std::string ReadSessionId(DWORD targetPid = 0)
    {
        HANDLE hMap = OpenOrCreateMapping(false, FILE_MAP_READ, targetPid);
        if (!hMap) return "";

        void* pBuf = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, sizeof(SessionData));
        if (!pBuf)
        {
            CloseHandle(hMap);
            return "";
        }

        char tempBuf[128] = { 0 };
        bool success = SafeReadSessionData(pBuf, tempBuf, sizeof(tempBuf));

        UnmapViewOfFile(pBuf);
        CloseHandle(hMap);

        if (!success) return "";
        return std::string(tempBuf);
    }
}
