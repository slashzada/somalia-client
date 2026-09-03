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

    // Rastreamento de ciclo de vida de callbacks
    bool EnterCallback();
    void LeaveCallback();
    bool WaitForCallbacks(DWORD timeoutMs = 5000);
    int GetActiveCallbacksCount();

    class CallbackGuard
    {
    public:
        CallbackGuard()
            : m_CanExecute(Main::EnterCallback())
        {
        }

        ~CallbackGuard()
        {
            Main::LeaveCallback();
        }

        CallbackGuard(const CallbackGuard&) = delete;
        CallbackGuard& operator=(const CallbackGuard&) = delete;

        bool IsActive() const
        {
            return m_CanExecute;
        }

    private:
        bool m_CanExecute;
    };
}
