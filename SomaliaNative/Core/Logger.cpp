#include "Logger.h"
#include <stdio.h>
#include <stdarg.h>

namespace Logger
{
    static FILE* s_LogFile = nullptr;

    void Initialize()
    {
        // Safe local logging to OutputDebugString and optional log file
        OutputDebugStringA("[SOMALIA] Logger inicializado.\n");
    }

    void Shutdown()
    {
        if (s_LogFile)
        {
            fclose(s_LogFile);
            s_LogFile = nullptr;
        }
    }

    void Log(const char* fmt, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        char outBuf[1100];
        snprintf(outBuf, sizeof(outBuf), "[SOMALIA] %s\n", buffer);
        OutputDebugStringA(outBuf);
        // Escrita em arquivo .log desabilitada a pedido do usuário
    }
}
