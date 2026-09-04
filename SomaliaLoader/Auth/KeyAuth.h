#pragma once
#include <string>
#include <vector>

struct KeyAuthUser
{
    std::string username = "User";
    std::string subscription = "Premium Lifetime";
    std::string expiry = "Nunca (Vitalício)";
    std::string daysLeft = "Ilimitado";
    std::string hwid = "";
    std::string ip = "127.0.0.1";
    std::string createDate = "";
    std::string lastLogin = "";
};

struct AuthResponse
{
    bool success = false;
    std::string message = "";
};

class KeyAuthClient
{
public:
    KeyAuthClient(const std::string& name, const std::string& ownerId, const std::string& secret, const std::string& version);

    bool Init();
    AuthResponse Login(const std::string& username, const std::string& password);
    AuthResponse Register(const std::string& username, const std::string& password, const std::string& key);
    bool SetUserVar(const std::string& varName, const std::string& varData);
    std::string GetUserVar(const std::string& varName);

    const KeyAuthUser& GetUser() const { return m_User; }
    const std::string& GetSessionId() const { return m_SessionId; }
    bool IsInitialized() const { return m_Initialized; }
    static std::string GetHWID();

private:
    static std::string UrlEncode(const std::string& value);
    std::string HttpPost(const std::string& postData);
    std::string ParseJsonField(const std::string& json, const std::string& key);

    std::string m_Name;
    std::string m_OwnerId;
    std::string m_Secret;
    std::string m_Version;
    std::string m_SessionId;
    bool m_Initialized;
    KeyAuthUser m_User;
};
