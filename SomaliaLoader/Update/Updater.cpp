#include "Updater.h"
#include <windows.h>
#include <wininet.h>
#include <shellapi.h>
#include <thread>
#include <atomic>
#include <sstream>
#include <vector>
#include <algorithm>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shell32.lib")

namespace Updater
{
    static UpdateInfo s_Info;
    static std::atomic<UpdateState> s_State{ UpdateState::Idle };
    static std::string s_StatusMessage = "Click to check for updates.";
    static std::atomic<float> s_DownloadProgress{ 0.0f };
    static std::string s_ManifestUrl = "https://raw.githubusercontent.com/slashzada/somalia-client/main/version.json";

    void Init()
    {
        s_Info.currentVersion = "1.0.0";
        s_State = UpdateState::Idle;
        s_StatusMessage = "Click to check for updates.";
        s_DownloadProgress = 0.0f;
    }

    void SetManifestUrl(const std::string& url)
    {
        if (!url.empty())
            s_ManifestUrl = url;
    }

    std::string GetManifestUrl()
    {
        return s_ManifestUrl;
    }

    static std::string ExtractJsonField(const std::string& json, const std::string& key)
    {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";

        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";

        size_t start = json.find_first_not_of(" \t\r\n", pos + 1);
        if (start == std::string::npos) return "";

        if (json[start] == '\"')
        {
            size_t end = json.find('\"', start + 1);
            if (end != std::string::npos)
                return json.substr(start + 1, end - start - 1);
        }
        else
        {
            size_t end = json.find_first_of(",}\r\n", start);
            if (end != std::string::npos)
                return json.substr(start, end - start);
        }
        return "";
    }

    static std::string FetchUrlString(const std::string& url)
    {
        std::string result;
        HINTERNET hInternet = InternetOpenA("SomaliaUpdater/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hInternet) return "";

        HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0,
            INTERNET_FLAG_RELOAD | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_NO_CACHE_WRITE, 0);

        if (hUrl)
        {
            char buffer[4096];
            DWORD bytesRead = 0;
            while (InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0)
            {
                buffer[bytesRead] = '\0';
                result.append(buffer, bytesRead);
            }
            InternetCloseHandle(hUrl);
        }
        InternetCloseHandle(hInternet);
        return result;
    }

    static std::vector<int> ParseVersionParts(const std::string& v)
    {
        std::vector<int> parts;
        std::stringstream ss(v);
        std::string item;
        while (std::getline(ss, item, '.'))
        {
            std::string numOnly;
            for (char c : item) { if (isdigit(c)) numOnly += c; }
            if (!numOnly.empty()) parts.push_back(std::stoi(numOnly));
            else parts.push_back(0);
        }
        while (parts.size() < 3) parts.push_back(0);
        return parts;
    }

    int CompareVersions(const std::string& v1, const std::string& v2)
    {
        std::vector<int> p1 = ParseVersionParts(v1);
        std::vector<int> p2 = ParseVersionParts(v2);

        size_t maxLen = (std::max)(p1.size(), p2.size());
        for (size_t i = 0; i < maxLen; i++)
        {
            int val1 = (i < p1.size()) ? p1[i] : 0;
            int val2 = (i < p2.size()) ? p2[i] : 0;
            if (val1 > val2) return 1;
            if (val1 < val2) return -1;
        }
        return 0;
    }

    void CheckForUpdatesAsync()
    {
        if (s_State == UpdateState::Checking || s_State == UpdateState::Downloading || s_State == UpdateState::Restarting)
            return;

        s_State = UpdateState::Checking;
        s_StatusMessage = "Checking for updates...";

        std::thread t([]()
        {
            std::string resp = FetchUrlString(s_ManifestUrl);
            if (resp.empty())
            {
                s_State = UpdateState::Error;
                s_StatusMessage = "Failed to connect to update server.";
                return;
            }

            // Tenta formato JSON
            std::string ver = ExtractJsonField(resp, "version");
            std::string url = ExtractJsonField(resp, "download_url");
            std::string log = ExtractJsonField(resp, "changelog");

            // Fallback caso formato seja texto simples: version|download_url|changelog
            if (ver.empty() && resp.find('|') != std::string::npos)
            {
                std::stringstream ss(resp);
                std::getline(ss, ver, '|');
                std::getline(ss, url, '|');
                std::getline(ss, log, '|');
            }

            // Remove quebras de linha e espaços extras
            ver.erase(std::remove_if(ver.begin(), ver.end(), [](char c) { return c == '\r' || c == '\n' || c == ' '; }), ver.end());
            url.erase(std::remove_if(url.begin(), url.end(), [](char c) { return c == '\r' || c == '\n' || c == ' '; }), url.end());

            if (ver.empty())
            {
                s_State = UpdateState::Error;
                s_StatusMessage = "Invalid response from server.";
                return;
            }

            s_Info.latestVersion = ver;
            s_Info.downloadUrl = url;
            s_Info.changelog = log;

            int comp = CompareVersions(s_Info.latestVersion, s_Info.currentVersion);
            if (comp > 0)
            {
                s_State = UpdateState::UpdateAvailable;
                s_StatusMessage = "New version v" + s_Info.latestVersion + " available!";
            }
            else
            {
                s_State = UpdateState::UpToDate;
                s_StatusMessage = "You are on the latest version (v" + s_Info.currentVersion + ").";
            }
        });
        t.detach();
    }

    void StartDownloadAndApplyAsync()
    {
        if (s_State == UpdateState::Downloading || s_State == UpdateState::Restarting)
            return;

        s_State = UpdateState::Downloading;
        s_StatusMessage = "Downloading update...";
        s_DownloadProgress = 0.0f;

        std::thread t([]()
        {
            if (s_Info.downloadUrl.empty())
            {
                s_State = UpdateState::Error;
                s_StatusMessage = "Download URL not available.";
                return;
            }

            char currentExePath[MAX_PATH] = { 0 };
            GetModuleFileNameA(NULL, currentExePath, MAX_PATH);

            std::string updateExePath = std::string(currentExePath) + ".update";

            HINTERNET hInternet = InternetOpenA("SomaliaUpdater/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
            if (!hInternet)
            {
                s_State = UpdateState::Error;
                s_StatusMessage = "Network error initializing download.";
                return;
            }

            HINTERNET hUrl = InternetOpenUrlA(hInternet, s_Info.downloadUrl.c_str(), NULL, 0,
                INTERNET_FLAG_RELOAD | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_NO_CACHE_WRITE, 0);
            if (!hUrl)
            {
                InternetCloseHandle(hInternet);
                s_State = UpdateState::Error;
                s_StatusMessage = "Failed to connect to download URL.";
                return;
            }

            DWORD contentLength = 0;
            DWORD sizeLength = sizeof(contentLength);
            DWORD headerIndex = 0;
            HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &sizeLength, &headerIndex);

            FILE* fp = fopen(updateExePath.c_str(), "wb");
            if (!fp)
            {
                InternetCloseHandle(hUrl);
                InternetCloseHandle(hInternet);
                s_State = UpdateState::Error;
                s_StatusMessage = "Failed to create update file on disk.";
                return;
            }

            char buffer[16384];
            DWORD bytesRead = 0;
            DWORD totalDownloaded = 0;
            bool success = true;

            while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
            {
                if (fwrite(buffer, 1, bytesRead, fp) != bytesRead)
                {
                    success = false;
                    break;
                }
                totalDownloaded += bytesRead;
                if (contentLength > 0)
                {
                    s_DownloadProgress = (float)totalDownloaded / (float)contentLength;
                }
                else
                {
                    s_DownloadProgress = 0.5f;
                }
            }

            fclose(fp);
            InternetCloseHandle(hUrl);
            InternetCloseHandle(hInternet);

            if (!success || totalDownloaded < 1024)
            {
                DeleteFileA(updateExePath.c_str());
                s_State = UpdateState::Error;
                s_StatusMessage = "Incomplete or corrupted download.";
                return;
            }

            s_DownloadProgress = 1.0f;
            s_State = UpdateState::Restarting;
            s_StatusMessage = "Installing version v" + s_Info.latestVersion + "...";

            // Script de substituição segura
            char cmd[1024];
            snprintf(cmd, sizeof(cmd),
                "/c ping 127.0.0.1 -n 2 > nul & move /y \"%s\" \"%s\" & start \"\" \"%s\"",
                updateExePath.c_str(), currentExePath, currentExePath);

            ShellExecuteA(NULL, "open", "cmd.exe", cmd, NULL, SW_HIDE);
            Sleep(400);
            ExitProcess(0);
        });
        t.detach();
    }

    UpdateState GetState()
    {
        return s_State.load();
    }

    std::string GetStatusMessage()
    {
        return s_StatusMessage;
    }

    float GetDownloadProgress()
    {
        return s_DownloadProgress.load();
    }

    const UpdateInfo& GetInfo()
    {
        return s_Info;
    }
}
