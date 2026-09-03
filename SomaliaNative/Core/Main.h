#pragma once
#include <windows.h>

enum class ShutdownState
{
    Running,
    StopRequested,
    Stopping,
    Stopped
};

namespace Main
{
    ShutdownState GetShutdownState();
    bool IsShuttingDown();
    bool IsStopRequested();
    void RequestUnload();
    void BeginShutdown();
    HMODULE GetModuleInstance();
    void UnloadAndExit();
    bool IsUnloaded();
}
