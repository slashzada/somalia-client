#pragma once
#include <string>

enum class UpdateState
{
    Idle,
    Checking,
    UpToDate,
    UpdateAvailable,
    Downloading,
    Restarting,
    Error
};

struct UpdateInfo
{
    std::string currentVersion = "1.0.0";
    std::string latestVersion = "";
    std::string downloadUrl = "";
    std::string changelog = "";
};

namespace Updater
{
    void Init();
    void SetManifestUrl(const std::string& url);
    std::string GetManifestUrl();

    void CheckForUpdatesAsync();
    void StartDownloadAndApplyAsync();

    UpdateState GetState();
    std::string GetStatusMessage();
    float GetDownloadProgress();
    const UpdateInfo& GetInfo();

    int CompareVersions(const std::string& v1, const std::string& v2);
}
