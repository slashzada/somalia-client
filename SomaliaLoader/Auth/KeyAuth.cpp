#include "KeyAuth.h"
#include <windows.h>
#include <wininet.h>
#include <sstream>
#include <ctime>
#include <cstdlib>

#pragma comment(lib, "wininet.lib")

KeyAuthClient::KeyAuthClient(const std::string& name, const std::string& ownerId, const std::string& secret, const std::string& version)
    : m_Name(name), m_OwnerId(ownerId), m_Secret(secret), m_Version(version), m_SessionId(""), m_Initialized(false)
{
    m_User.hwid = GetHWID();
}

std::string KeyAuthClient::GetHWID()
{
    char buffer[256] = { 0 };
    DWORD bufferSize = sizeof(buffer);
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
    {
        RegQueryValueExA(hKey, "MachineGuid", NULL, NULL, reinterpret_cast<LPBYTE>(buffer), &bufferSize);
        RegCloseKey(hKey);
    }
    if (buffer[0] != '\0')
    {
        return std::string(buffer);
    }
    // Fallback para nome do computador
    DWORD compLen = sizeof(buffer);
    GetComputerNameA(buffer, &compLen);
    return std::string(buffer);
}

std::string KeyAuthClient::HttpPost(const std::string& postData)
{
    std::string response;
    HINTERNET hInternet = InternetOpenA("SomaliaClient/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return "";

    HINTERNET hConnect = InternetConnectA(hInternet, "keyauth.win", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect)
    {
        InternetCloseHandle(hInternet);
        return "";
    }

    const char* acceptTypes[] = { "*/*", NULL };
    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", "/api/1.2/", NULL, NULL, acceptTypes, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);
    if (!hRequest)
    {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return "";
    }

    std::string headers = "Content-Type: application/x-www-form-urlencoded\r\n";
    BOOL sent = HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.length(), (LPVOID)postData.c_str(), (DWORD)postData.length());

    if (sent)
    {
        char buffer[4096];
        DWORD bytesRead = 0;
        while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            response.append(buffer, bytesRead);
        }
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return response;
}

std::string KeyAuthClient::ParseJsonField(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    size_t colon = json.find(':', pos + search.length());
    if (colon == std::string::npos) return "";

    size_t start = json.find_first_not_of(" \t\r\n", colon + 1);
    if (start == std::string::npos) return "";

    if (json[start] == '\"')
    {
        size_t end = json.find('\"', start + 1);
        if (end != std::string::npos)
        {
            return json.substr(start + 1, end - start - 1);
        }
    }
    else
    {
        size_t end = json.find_first_of(",}\r\n", start);
        if (end != std::string::npos)
        {
            return json.substr(start, end - start);
        }
    }
    return "";
}

bool KeyAuthClient::Init()
{
    const char* allowDev = std::getenv("SOMALIA_ALLOW_DEV_AUTH");
    const bool devAuthAllowed = allowDev && std::string(allowDev) == "1";

    if (m_Name.empty() || m_Name == "YOUR_APP_NAME" || m_Secret.empty() || m_Secret == "YOUR_SECRET")
    {
        m_Initialized = false;
        m_SessionId = "";
        return false;
    }

    std::string body = "type=init&name=" + m_Name + "&ownerid=" + m_OwnerId + "&secret=" + m_Secret + "&ver=" + m_Version;
    std::string resp = HttpPost(body);

    if (resp.find("\"success\":true") != std::string::npos || resp.find("\"success\": true") != std::string::npos)
    {
        m_SessionId = ParseJsonField(resp, "sessionid");
        m_Initialized = true;
        return true;
    }

    if (devAuthAllowed)
    {
        m_Initialized = true;
        m_SessionId = "dev_session";
        return true;
    }

    m_Initialized = false;
    m_SessionId = "";
    return false;
}

AuthResponse KeyAuthClient::Login(const std::string& username, const std::string& password)
{
    if (username.empty() || password.empty())
    {
        return { false, "Preencha usuario e senha." };
    }

    if (!m_Initialized || m_SessionId.empty())
    {
        return { false, "Autenticacao indisponivel. Verifique a configuracao do KeyAuth." };
    }

    if (m_SessionId == "dev_session")
    {
        m_User.username = username;
        m_User.subscription = "VIP Lifetime";
        m_User.expiry = "Vitalicio";
        m_User.daysLeft = "Ilimitado";
        return { true, "Login realizado com sucesso! (Modo Dev)" };
    }

    std::string body = "type=login&username=" + username + "&pass=" + password + "&hwid=" + m_User.hwid +
                       "&sessionid=" + m_SessionId + "&name=" + m_Name + "&ownerid=" + m_OwnerId;
    std::string resp = HttpPost(body);

    if (resp.find("\"success\":true") != std::string::npos || resp.find("\"success\": true") != std::string::npos)
    {
        m_User.username = username;

        std::string sub = ParseJsonField(resp, "subscription");
        if (!sub.empty()) m_User.subscription = sub;

        std::string expStr = ParseJsonField(resp, "expiry");
        if (!expStr.empty())
        {
            try
            {
                long long expTime = std::stoll(expStr);
                if (expTime > 0)
                {
                    time_t now = time(nullptr);
                    long long diffSec = expTime - now;
                    if (diffSec > 0)
                    {
                        int days = static_cast<int>(diffSec / (24 * 3600));
                        int hours = static_cast<int>((diffSec % (24 * 3600)) / 3600);
                        m_User.daysLeft = std::to_string(days) + "d " + std::to_string(hours) + "h restantes";

                        tm t;
                        time_t tt = (time_t)expTime;
                        localtime_s(&t, &tt);
                        char dateBuf[64];
                        strftime(dateBuf, sizeof(dateBuf), "%d/%m/%Y %H:%M", &t);
                        m_User.expiry = dateBuf;
                    }
                    else
                    {
                        return { false, "Sua licenca expirou." };
                    }
                }
                else
                {
                    m_User.expiry = "Vitalicio";
                    m_User.daysLeft = "Ilimitado";
                }
            }
            catch (...)
            {
                m_User.expiry = "Vitalicio";
                m_User.daysLeft = "Ativo";
            }
        }

        std::string msg = ParseJsonField(resp, "message");
        return { true, msg.empty() ? "Autenticado com sucesso!" : msg };
    }

    std::string msg = ParseJsonField(resp, "message");
    return { false, msg.empty() ? "Falha ao autenticar usuario ou senha." : msg };
}

AuthResponse KeyAuthClient::Register(const std::string& username, const std::string& password, const std::string& key)
{
    if (username.empty() || password.empty() || key.empty())
    {
        return { false, "Preencha usuario, senha e chave de licenca." };
    }

    if (!m_Initialized || m_SessionId.empty())
    {
        return { false, "Registro indisponivel. Verifique a configuracao do KeyAuth." };
    }

    if (m_SessionId == "dev_session")
    {
        m_User.username = username;
        m_User.subscription = "VIP Lifetime";
        m_User.expiry = "Vitalicio";
        m_User.daysLeft = "Ilimitado";
        return { true, "Conta registrada com sucesso! (Modo Dev)" };
    }

    std::string body = "type=register&username=" + username + "&pass=" + password + "&key=" + key +
                       "&hwid=" + m_User.hwid + "&sessionid=" + m_SessionId + "&name=" + m_Name + "&ownerid=" + m_OwnerId;
    std::string resp = HttpPost(body);

    if (resp.find("\"success\":true") != std::string::npos || resp.find("\"success\": true") != std::string::npos)
    {
        m_User.username = username;
        return { true, "Registro efetuado com sucesso! Faca login." };
    }

    std::string msg = ParseJsonField(resp, "message");
    return { false, msg.empty() ? "Falha ao registrar conta. Chave invalida." : msg };
}

bool KeyAuthClient::SetUserVar(const std::string& varName, const std::string& varData)
{
    if (m_SessionId.empty()) return false;

    // Escapa a data se necessário para x-www-form-urlencoded
    std::string postData = "type=setvar&var=" + varName + "&data=" + varData +
                           "&sessionid=" + m_SessionId + "&name=" + m_Name + "&ownerid=" + m_OwnerId;
    std::string resp = HttpPost(postData);
    return (resp.find("\"success\":true") != std::string::npos || resp.find("\"success\": true") != std::string::npos);
}

std::string KeyAuthClient::GetUserVar(const std::string& varName)
{
    if (m_SessionId.empty()) return "";

    std::string postData = "type=getvar&var=" + varName +
                           "&sessionid=" + m_SessionId + "&name=" + m_Name + "&ownerid=" + m_OwnerId;
    std::string resp = HttpPost(postData);

    if (resp.find("\"success\":true") != std::string::npos || resp.find("\"success\": true") != std::string::npos)
    {
        return ParseJsonField(resp, "response");
    }
    return "";
}
