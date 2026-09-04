#include "LoaderConfig.h"
#include "../../Common/MiniJson.h"
#include "../../Common/SharedSession.h"
#include <fstream>
#include <sstream>
#include <windows.h>
#include <cstdlib>

namespace ConfigManager
{
    static LoaderConfig s_Config;
    static HANDLE s_SharedSessionHandle = nullptr;
    static HANDLE s_PidSessionHandle = nullptr;

    static std::string GetEnvValue(const char* name)
    {
        char* value = nullptr;
        size_t len = 0;
        if (_dupenv_s(&value, &len, name) != 0 || value == nullptr)
        {
            return "";
        }

        std::string result(value);
        free(value);
        return result;
    }

    static void ApplyKeyAuthEnvOverrides()
    {
        std::string envName = GetEnvValue("SOMALIA_KEYAUTH_NAME");
        if (!envName.empty()) s_Config.keyauthName = envName;

        std::string envOwner = GetEnvValue("SOMALIA_KEYAUTH_OWNER");
        if (!envOwner.empty()) s_Config.keyauthOwner = envOwner;

        // Secret fornecida estritamente em runtime / memoria
        std::string envSecret = GetEnvValue("SOMALIA_KEYAUTH_SECRET");
        if (!envSecret.empty()) s_Config.keyauthSecret = envSecret;

        std::string envVersion = GetEnvValue("SOMALIA_KEYAUTH_VERSION");
        if (!envVersion.empty()) s_Config.keyauthVersion = envVersion;
    }

    LoaderConfig& Get()
    {
        return s_Config;
    }

    void PublishSession(const std::string& sessionId)
    {
        s_Config.sessionId = sessionId;
        SharedSession::PublishSession(sessionId, s_SharedSessionHandle, 0);
    }

    void PublishSessionForPid(const std::string& sessionId, DWORD pid)
    {
        SharedSession::PublishSession(sessionId, s_PidSessionHandle, pid);
    }

    void RevokeSession()
    {
        s_Config.sessionId.clear();
        SharedSession::RevokeSession(s_SharedSessionHandle);
        SharedSession::RevokeSession(s_PidSessionHandle);
    }

    void Shutdown()
    {
        RevokeSession();
    }

    bool Load(const std::string& filePath)
    {
        std::ifstream f(filePath);
        if (!f.is_open())
        {
            ApplyKeyAuthEnvOverrides();
            return false;
        }

        std::stringstream ss;
        ss << f.rdbuf();
        std::string content = ss.str();
        f.close();

        MiniJson::JsonValue root;
        std::string err;
        if (MiniJson::Parser::Parse(content, root, err) && root.is_object())
        {
            s_Config.gtaPath = root.get_string("gta_path", s_Config.gtaPath);
            s_Config.rememberUser = root.get_bool("remember_user", s_Config.rememberUser);
            s_Config.lastUsername = root.get_string("last_username", s_Config.lastUsername);
            s_Config.userSubscription = root.get_string("user_subscription", s_Config.userSubscription);
            s_Config.userExpiry = root.get_string("user_expiry", s_Config.userExpiry);
            s_Config.userDaysLeft = root.get_string("user_days_left", s_Config.userDaysLeft);

            s_Config.keyauthName = root.get_string("keyauth_name", s_Config.keyauthName);
            s_Config.keyauthOwner = root.get_string("keyauth_owner", s_Config.keyauthOwner);
            s_Config.keyauthVersion = root.get_string("keyauth_version", s_Config.keyauthVersion);

            // NUNCA carregar session_id ou keyauth_secret de arquivo local.
            // A sessao sempre comeca nula e exige nova autenticacao a cada execucao.
            s_Config.sessionId = "";
        }

        ApplyKeyAuthEnvOverrides();
        return true;
    }

    bool Save(const std::string& filePath)
    {
        MiniJson::JsonValue root(MiniJson::Type::Object);
        root["gta_path"] = s_Config.gtaPath;
        root["remember_user"] = s_Config.rememberUser;
        root["last_username"] = s_Config.rememberUser ? s_Config.lastUsername : "";
        root["user_subscription"] = s_Config.userSubscription;
        root["user_expiry"] = s_Config.userExpiry;
        root["user_days_left"] = s_Config.userDaysLeft;

        // Metadados publicos/configuraveis do app KeyAuth
        root["keyauth_name"] = s_Config.keyauthName;
        root["keyauth_owner"] = s_Config.keyauthOwner;
        root["keyauth_version"] = s_Config.keyauthVersion;

        // SEGURANCA: NUNCA persistir keyauth_secret, session_id, senha ou tokens em disco
        std::string jsonStr = root.serialize(true);

        std::string tmpPath = filePath + ".tmp";
        std::ofstream f(tmpPath, std::ios::trunc);
        if (!f.is_open())
        {
            std::ofstream fDirect(filePath, std::ios::trunc);
            if (!fDirect.is_open()) return false;
            fDirect << jsonStr;
            return true;
        }

        f << jsonStr;
        f.close();

        if (!MoveFileExA(tmpPath.c_str(), filePath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
        {
            DeleteFileA(filePath.c_str());
            MoveFileA(tmpPath.c_str(), filePath.c_str());
        }

        return true;
    }
}
