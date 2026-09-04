#include "KeyAuth.h"
#include "../../Common/MiniJson.h"
#include <windows.h>
#include <wininet.h>
#include <wincrypt.h>
#include <intrin.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "advapi32.lib")

std::string KeyAuthClient::UrlEncode(const std::string& value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value)
    {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            escaped << c;
        }
        else
        {
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
            escaped << std::nouppercase;
        }
    }

    return escaped.str();
}

std::string KeyAuthClient::ComputeSHA256(const std::string& input)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::string result = "";

    if (!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
    {
        if (!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        {
            return "";
        }
    }

    if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
    {
        if (CryptHashData(hHash, reinterpret_cast<const BYTE*>(input.data()), static_cast<DWORD>(input.size()), 0))
        {
            BYTE hashBuf[32] = { 0 };
            DWORD hashLen = sizeof(hashBuf);
            if (CryptGetHashParam(hHash, HP_HASHVAL, hashBuf, &hashLen, 0))
            {
                char hex[65] = { 0 };
                for (DWORD i = 0; i < hashLen; ++i)
                {
                    snprintf(hex + (i * 2), 3, "%02x", hashBuf[i]);
                }
                result = std::string(hex);
            }
        }
        CryptDestroyHash(hHash);
    }

    CryptReleaseContext(hProv, 0);
    return result;
}

std::string KeyAuthClient::GetHWID()
{
    std::string machineGuid = "";
    char guidBuf[256] = { 0 };
    DWORD guidSize = sizeof(guidBuf);
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExA(hKey, "MachineGuid", NULL, NULL, reinterpret_cast<LPBYTE>(guidBuf), &guidSize) == ERROR_SUCCESS)
        {
            machineGuid = guidBuf;
        }
        RegCloseKey(hKey);
    }

    DWORD volSerial = 0;
    char sysDir[MAX_PATH] = { 0 };
    if (GetSystemDirectoryA(sysDir, MAX_PATH) > 0)
    {
        char rootPath[4] = { sysDir[0], ':', '\\', '\0' };
        GetVolumeInformationA(rootPath, NULL, 0, &volSerial, NULL, NULL, NULL, 0);
    }

    int cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 0);
    int cpuSig = 0;
    int cpuFeatures = 0;
    if (cpuInfo[0] >= 1)
    {
        int cpuData[4] = { 0 };
        __cpuid(cpuData, 1);
        cpuSig = cpuData[0];
        cpuFeatures = cpuData[3];
    }

    char compName[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
    DWORD compLen = sizeof(compName);
    GetComputerNameA(compName, &compLen);

    bool hasEntropy = (!machineGuid.empty()) || (volSerial != 0 && cpuSig != 0);
    if (!hasEntropy)
    {
        return "";
    }

    char volHex[16] = { 0 };
    snprintf(volHex, sizeof(volHex), "%08X", volSerial);

    std::string rawData = "MG=" + machineGuid +
                          ";VS=" + volHex +
                          ";CPU=" + std::to_string(cpuSig) + ":" + std::to_string(cpuFeatures) +
                          ";CN=" + compName;

    std::string hashed = ComputeSHA256(rawData);
    if (!hashed.empty())
    {
        return hashed;
    }

    // Fallback normalizado caso o CryptoAPI falhe
    return machineGuid.empty() ? std::string(volHex) : machineGuid;
}

KeyAuthClient::KeyAuthClient(const std::string& name, const std::string& ownerId, const std::string& secret, const std::string& version)
    : m_Name(name), m_OwnerId(ownerId), m_Secret(secret), m_Version(version), m_SessionId(""), m_Initialized(false)
{
    m_User.hwid = GetHWID();
}

KeyAuthClient::~KeyAuthClient()
{
    ClearSession();
}

void KeyAuthClient::ClearSession()
{
    m_SessionId.clear();
    m_Initialized = false;
    SecureZeroMemory(&m_User, sizeof(m_User));
    m_User.username = "User";
}

HttpResponse KeyAuthClient::HttpPost(const std::string& postData)
{
    HttpResponse res;

    HINTERNET hInternet = InternetOpenA("SomaliaClient/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet)
    {
        res.status = HttpStatus::NetworkError;
        res.win32Error = GetLastError();
        res.errorMessage = "Falha ao inicializar subsistema WinINet.";
        return res;
    }

    DWORD timeoutMs = 6000;
    InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeoutMs, sizeof(timeoutMs));
    InternetSetOptionA(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeoutMs, sizeof(timeoutMs));
    InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeoutMs, sizeof(timeoutMs));

    HINTERNET hConnect = InternetConnectA(hInternet, "keyauth.win", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect)
    {
        DWORD err = GetLastError();
        InternetCloseHandle(hInternet);
        res.win32Error = err;
        if (err == ERROR_INTERNET_NAME_NOT_RESOLVED)
        {
            res.status = HttpStatus::DnsError;
            res.errorMessage = "Falha de resolucao DNS para keyauth.win.";
        }
        else if (err == ERROR_INTERNET_CANNOT_CONNECT)
        {
            res.status = HttpStatus::ConnectionRefused;
            res.errorMessage = "Conexao recusada pelo servidor.";
        }
        else if (err == ERROR_INTERNET_TIMEOUT)
        {
            res.status = HttpStatus::Timeout;
            res.errorMessage = "Tempo limite de conexao excedido.";
        }
        else
        {
            res.status = HttpStatus::NetworkError;
            res.errorMessage = "Falha ao conectar com o servidor.";
        }
        return res;
    }

    const char* acceptTypes[] = { "*/*", NULL };
    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", "/api/1.2/", NULL, NULL, acceptTypes, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);
    if (!hRequest)
    {
        DWORD err = GetLastError();
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        res.status = HttpStatus::NetworkError;
        res.win32Error = err;
        res.errorMessage = "Falha ao abrir requisicao HTTP.";
        return res;
    }

    std::string headers = "Content-Type: application/x-www-form-urlencoded\r\n";
    BOOL sent = HttpSendRequestA(hRequest, headers.c_str(), static_cast<DWORD>(headers.length()),
                                 const_cast<char*>(postData.c_str()), static_cast<DWORD>(postData.length()));

    if (!sent)
    {
        DWORD err = GetLastError();
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        res.win32Error = err;
        if (err == ERROR_INTERNET_TIMEOUT)
        {
            res.status = HttpStatus::Timeout;
            res.errorMessage = "Tempo limite na transmissao de dados.";
        }
        else if (err == ERROR_INTERNET_SECURITY_CHANNEL_ERROR ||
                 err == ERROR_INTERNET_SEC_CERT_DATE_INVALID ||
                 err == ERROR_INTERNET_SEC_CERT_CN_INVALID ||
                 err == ERROR_INTERNET_INVALID_CA)
        {
            res.status = HttpStatus::NetworkError;
            res.errorMessage = "Falha na negociacao TLS/SSL com o servidor.";
        }
        else
        {
            res.status = HttpStatus::NetworkError;
            res.errorMessage = "Falha ao enviar requisicao ao servidor.";
        }
        return res;
    }

    DWORD statusCode = 0;
    DWORD scSize = sizeof(statusCode);
    if (HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &scSize, NULL))
    {
        res.httpStatusCode = statusCode;
        if (statusCode >= 400)
        {
            res.status = HttpStatus::HttpError;
            res.errorMessage = "Servidor respondeu com status HTTP " + std::to_string(statusCode);
        }
    }

    char buffer[4096];
    DWORD bytesRead = 0;
    while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        res.body.append(buffer, bytesRead);
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    if (res.body.empty() && res.status != HttpStatus::HttpError)
    {
        res.status = HttpStatus::EmptyResponse;
        res.errorMessage = "Resposta vazia recebida do servidor.";
    }
    else if (res.status != HttpStatus::HttpError)
    {
        res.status = HttpStatus::Success;
    }

    return res;
}

bool KeyAuthClient::Init()
{
#ifdef _DEBUG
    const char* allowDev = std::getenv("SOMALIA_ALLOW_DEV_AUTH");
    const bool devAuthAllowed = allowDev && std::string(allowDev) == "1";
#endif

    if (m_Name.empty() || m_Name == "YOUR_APP_NAME" || m_Secret.empty() || m_Secret == "YOUR_SECRET")
    {
#ifdef _DEBUG
        if (devAuthAllowed)
        {
            m_Initialized = true;
            m_SessionId = "dev_session";
            return true;
        }
#endif
        m_Initialized = false;
        m_SessionId = "";
        return false;
    }

    std::string body = "type=init&name=" + UrlEncode(m_Name) + "&ownerid=" + UrlEncode(m_OwnerId) + "&secret=" + UrlEncode(m_Secret) + "&ver=" + UrlEncode(m_Version);
    HttpResponse resp = HttpPost(body);

    if (resp.status == HttpStatus::Success && !resp.body.empty())
    {
        MiniJson::JsonValue root;
        std::string jsonErr;
        if (MiniJson::Parser::Parse(resp.body, root, jsonErr) && root.is_object())
        {
            if (root.get_bool("success"))
            {
                m_SessionId = root.get_string("sessionid");
                m_Initialized = !m_SessionId.empty();
                if (m_Initialized) return true;
            }
        }
    }

#ifdef _DEBUG
    if (devAuthAllowed)
    {
        m_Initialized = true;
        m_SessionId = "dev_session";
        return true;
    }
#endif

    m_Initialized = false;
    m_SessionId = "";
    return false;
}

AuthResponse KeyAuthClient::Login(const std::string& username, const std::string& password)
{
    if (username.empty() || password.empty())
    {
        return { false, "Preencha usuario e senha.", HttpStatus::Success };
    }

    if (!m_Initialized || m_SessionId.empty())
    {
        if (!Init() || m_SessionId.empty())
        {
            return { false, "Autenticacao indisponivel. Verifique a conexao e configuracao do KeyAuth.", HttpStatus::NetworkError };
        }
    }

#ifdef _DEBUG
    if (m_SessionId == "dev_session")
    {
        m_User.username = username;
        m_User.subscription = "VIP Lifetime (Debug)";
        m_User.expiry = "Vitalicio";
        m_User.daysLeft = "Ilimitado";
        return { true, "Login realizado com sucesso! (Modo Debug)", HttpStatus::Success };
    }
#endif

    std::string body = "type=login&username=" + UrlEncode(username) + "&pass=" + UrlEncode(password) + "&hwid=" + UrlEncode(m_User.hwid) +
                       "&sessionid=" + UrlEncode(m_SessionId) + "&name=" + UrlEncode(m_Name) + "&ownerid=" + UrlEncode(m_OwnerId);
    HttpResponse resp = HttpPost(body);

    if (resp.status != HttpStatus::Success || resp.body.empty())
    {
        std::string err = resp.errorMessage.empty() ? "Falha de comunicacao com o servidor de autenticacao." : resp.errorMessage;
        return { false, err, resp.status };
    }

    MiniJson::JsonValue root;
    std::string jsonErr;
    if (!MiniJson::Parser::Parse(resp.body, root, jsonErr) || !root.is_object())
    {
        return { false, "Resposta invalida do servidor de autenticacao.", HttpStatus::InvalidResponse };
    }

    if (root.get_bool("success"))
    {
        m_User.username = username;

        std::string sub = root.get_string("subscription");
        if (!sub.empty()) m_User.subscription = sub;

        std::string expStr = root.get_string("expiry");
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
                        time_t tt = static_cast<time_t>(expTime);
                        localtime_s(&t, &tt);
                        char dateBuf[64];
                        strftime(dateBuf, sizeof(dateBuf), "%d/%m/%Y %H:%M", &t);
                        m_User.expiry = dateBuf;
                    }
                    else
                    {
                        return { false, "Sua licenca expirou.", HttpStatus::Success };
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

        std::string msg = root.get_string("message");
        return { true, msg.empty() ? "Autenticado com sucesso!" : msg, HttpStatus::Success };
    }

    std::string msg = root.get_string("message");
    return { false, msg.empty() ? "Falha ao autenticar usuario ou senha." : msg, HttpStatus::Success };
}

AuthResponse KeyAuthClient::Register(const std::string& username, const std::string& password, const std::string& key)
{
    if (username.empty() || password.empty() || key.empty())
    {
        return { false, "Preencha usuario, senha e chave de licenca.", HttpStatus::Success };
    }

    if (!m_Initialized || m_SessionId.empty())
    {
        if (!Init() || m_SessionId.empty())
        {
            return { false, "Registro indisponivel. Verifique a conexao e configuracao do KeyAuth.", HttpStatus::NetworkError };
        }
    }

#ifdef _DEBUG
    if (m_SessionId == "dev_session")
    {
        m_User.username = username;
        m_User.subscription = "VIP Lifetime (Debug)";
        m_User.expiry = "Vitalicio";
        m_User.daysLeft = "Ilimitado";
        return { true, "Conta registrada com sucesso! (Modo Debug)", HttpStatus::Success };
    }
#endif

    std::string body = "type=register&username=" + UrlEncode(username) + "&pass=" + UrlEncode(password) + "&key=" + UrlEncode(key) +
                       "&hwid=" + UrlEncode(m_User.hwid) + "&sessionid=" + UrlEncode(m_SessionId) + "&name=" + UrlEncode(m_Name) + "&ownerid=" + UrlEncode(m_OwnerId);
    HttpResponse resp = HttpPost(body);

    if (resp.status != HttpStatus::Success || resp.body.empty())
    {
        std::string err = resp.errorMessage.empty() ? "Falha de comunicacao com o servidor de autenticacao." : resp.errorMessage;
        return { false, err, resp.status };
    }

    MiniJson::JsonValue root;
    std::string jsonErr;
    if (!MiniJson::Parser::Parse(resp.body, root, jsonErr) || !root.is_object())
    {
        return { false, "Resposta invalida do servidor de autenticacao.", HttpStatus::InvalidResponse };
    }

    if (root.get_bool("success"))
    {
        m_User.username = username;
        std::string msg = root.get_string("message");
        return { true, msg.empty() ? "Registro efetuado com sucesso! Faca login." : msg, HttpStatus::Success };
    }

    std::string msg = root.get_string("message");
    return { false, msg.empty() ? "Falha ao registrar conta. Chave invalida." : msg, HttpStatus::Success };
}

bool KeyAuthClient::SetUserVar(const std::string& varName, const std::string& varData)
{
    if (m_SessionId.empty()) return false;

    std::string postData = "type=setvar&var=" + UrlEncode(varName) + "&data=" + UrlEncode(varData) +
                           "&sessionid=" + UrlEncode(m_SessionId) + "&name=" + UrlEncode(m_Name) + "&ownerid=" + UrlEncode(m_OwnerId);
    HttpResponse resp = HttpPost(postData);
    if (resp.status != HttpStatus::Success || resp.body.empty()) return false;

    MiniJson::JsonValue root;
    std::string jsonErr;
    if (MiniJson::Parser::Parse(resp.body, root, jsonErr) && root.is_object())
    {
        return root.get_bool("success");
    }
    return false;
}

std::string KeyAuthClient::GetUserVar(const std::string& varName)
{
    if (m_SessionId.empty()) return "";

    std::string postData = "type=getvar&var=" + UrlEncode(varName) +
                           "&sessionid=" + UrlEncode(m_SessionId) + "&name=" + UrlEncode(m_Name) + "&ownerid=" + UrlEncode(m_OwnerId);
    HttpResponse resp = HttpPost(postData);
    if (resp.status != HttpStatus::Success || resp.body.empty()) return "";

    MiniJson::JsonValue root;
    std::string jsonErr;
    if (MiniJson::Parser::Parse(resp.body, root, jsonErr) && root.is_object() && root.get_bool("success"))
    {
        return root.get_string("response");
    }
    return "";
}
