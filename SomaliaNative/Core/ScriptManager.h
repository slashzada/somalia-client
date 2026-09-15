#pragma once
#include <windows.h>
#include <string>

namespace ScriptManager
{
    void Deploy();
    void Cleanup();
    void TriggerMoonLoaderReload();
    std::string GetGtaDirectory();
}
