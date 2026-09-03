#pragma once
#include <windows.h>
#include <string>

namespace Logger
{
    void Initialize();
    void Shutdown();
    void Log(const char* fmt, ...);
}
