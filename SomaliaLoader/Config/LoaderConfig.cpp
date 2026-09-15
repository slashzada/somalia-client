#include "LoaderConfig.h"
#include <windows.h>

namespace ConfigManager
{
    static LoaderConfig s_Config;

    LoaderConfig& Get()
    {
        return s_Config;
    }

    static std::string ReadRegString(HKEY root, const char* subKey, const char* valName)
    {
        HKEY hKey;
        if (RegOpenKeyExA(root, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
            return "";
        char buf[512] = { 0 };
        DWORD sz = sizeof(buf);
        DWORD type = REG_SZ;
        if (RegQueryValueExA(hKey, valName, NULL, &type, reinterpret_cast<LPBYTE>(buf), &sz) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return "";
        }
        RegCloseKey(hKey);
        return std::string(buf);
    }

    static void CleanJsonFiles()
    {
        // Deleta qualquer arquivo somalia_client.json no disco para nunca expor credenciais
        DeleteFileA("somalia_client.json");
        char tempPath[MAX_PATH] = { 0 };
        if (GetTempPathA(MAX_PATH, tempPath))
        {
            std::string tmpFile = std::string(tempPath) + "somalia_client.json";
            DeleteFileA(tmpFile.c_str());
        }
        if (!s_Config.gtaPath.empty())
        {
            std::string gtaFile = s_Config.gtaPath + "\\somalia_client.json";
            DeleteFileA(gtaFile.c_str());
        }
    }

    bool Load(const std::string& filePath)
    {
        // Remove arquivos json antigos que possam ter vazado credenciais
        CleanJsonFiles();

        std::string gta = ReadRegString(HKEY_CURRENT_USER, "Software\\SomaliaClient", "gta_path");
        if (!gta.empty()) s_Config.gtaPath = gta;

        std::string lastUser = ReadRegString(HKEY_CURRENT_USER, "Software\\SomaliaClient", "last_username");
        if (!lastUser.empty()) s_Config.lastUsername = lastUser;

        std::string rem = ReadRegString(HKEY_CURRENT_USER, "Software\\SomaliaClient", "remember_user");
        if (!rem.empty()) s_Config.rememberUser = (rem == "true" || rem == "1");

        std::string sub = ReadRegString(HKEY_CURRENT_USER, "Software\\SomaliaClient", "user_subscription");
        if (!sub.empty()) s_Config.userSubscription = sub;

        std::string exp = ReadRegString(HKEY_CURRENT_USER, "Software\\SomaliaClient", "user_expiry");
        if (!exp.empty()) s_Config.userExpiry = exp;

        std::string days = ReadRegString(HKEY_CURRENT_USER, "Software\\SomaliaClient", "user_days_left");
        if (!days.empty()) s_Config.userDaysLeft = days;

        std::string sid = ReadRegString(HKEY_CURRENT_USER, "Software\\SomaliaClient", "session_id");
        if (!sid.empty()) s_Config.sessionId = sid;

        std::string sp = ReadRegString(HKEY_CURRENT_USER, "Software\\SomaliaClient", "stream_proof");
        if (!sp.empty()) s_Config.streamProof = (sp == "true" || sp == "1");

        return true;
    }

    static void SaveSessionToRegistry(const LoaderConfig& cfg)
    {
        HKEY hKey;
        if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\SomaliaClient", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
        {
            if (!cfg.gtaPath.empty())
                RegSetValueExA(hKey, "gta_path", 0, REG_SZ, reinterpret_cast<const BYTE*>(cfg.gtaPath.c_str()), (DWORD)cfg.gtaPath.length() + 1);

            const char* remStr = cfg.rememberUser ? "true" : "false";
            RegSetValueExA(hKey, "remember_user", 0, REG_SZ, reinterpret_cast<const BYTE*>(remStr), (DWORD)strlen(remStr) + 1);

            if (!cfg.sessionId.empty())
                RegSetValueExA(hKey, "session_id", 0, REG_SZ, reinterpret_cast<const BYTE*>(cfg.sessionId.c_str()), (DWORD)cfg.sessionId.length() + 1);
            if (!cfg.lastUsername.empty())
                RegSetValueExA(hKey, "last_username", 0, REG_SZ, reinterpret_cast<const BYTE*>(cfg.lastUsername.c_str()), (DWORD)cfg.lastUsername.length() + 1);
            if (!cfg.userSubscription.empty())
                RegSetValueExA(hKey, "user_subscription", 0, REG_SZ, reinterpret_cast<const BYTE*>(cfg.userSubscription.c_str()), (DWORD)cfg.userSubscription.length() + 1);
            if (!cfg.userExpiry.empty())
                RegSetValueExA(hKey, "user_expiry", 0, REG_SZ, reinterpret_cast<const BYTE*>(cfg.userExpiry.c_str()), (DWORD)cfg.userExpiry.length() + 1);
            if (!cfg.userDaysLeft.empty())
                RegSetValueExA(hKey, "user_days_left", 0, REG_SZ, reinterpret_cast<const BYTE*>(cfg.userDaysLeft.c_str()), (DWORD)cfg.userDaysLeft.length() + 1);
            if (!cfg.keyauthName.empty())
                RegSetValueExA(hKey, "keyauth_name", 0, REG_SZ, reinterpret_cast<const BYTE*>(cfg.keyauthName.c_str()), (DWORD)cfg.keyauthName.length() + 1);
            if (!cfg.keyauthOwner.empty())
                RegSetValueExA(hKey, "keyauth_owner", 0, REG_SZ, reinterpret_cast<const BYTE*>(cfg.keyauthOwner.c_str()), (DWORD)cfg.keyauthOwner.length() + 1);

            const char* spStr = cfg.streamProof ? "true" : "false";
            RegSetValueExA(hKey, "stream_proof", 0, REG_SZ, reinterpret_cast<const BYTE*>(spStr), (DWORD)strlen(spStr) + 1);

            // NUNCA salva keyauth_secret em disco ou registro. O secret fica compilado de forma segura no binario.
            RegCloseKey(hKey);
        }
    }

    bool Save(const std::string& filePath)
    {
        // Salva apenas na sessao de Registro do Windows
        SaveSessionToRegistry(s_Config);

        // Deleta qualquer arquivo somalia_client.json para proteger as credenciais contra vazamento
        CleanJsonFiles();
        if (!filePath.empty() && filePath != "somalia_client.json")
        {
            DeleteFileA(filePath.c_str());
        }

        return true;
    }
}
