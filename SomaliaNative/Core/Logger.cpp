#include "Logger.h"
#include <stdio.h>
#include <stdarg.h>

namespace Logger
{
    static FILE* s_LogFile = nullptr;
    static SRWLOCK s_LogLock = SRWLOCK_INIT;

    void Initialize()
    {
        AcquireSRWLockExclusive(&s_LogLock);
#ifdef _DEBUG
        s_LogFile = fopen("somalia_debug.log", "w");
        OutputDebugStringA("[SOMALIA] Logger inicializado.\n");
        if (s_LogFile)
        {
            fputs("[SOMALIA] Logger inicializado.\n", s_LogFile);
            fflush(s_LogFile);
        }
#endif
        ReleaseSRWLockExclusive(&s_LogLock);
    }

    void Shutdown()
    {
        AcquireSRWLockExclusive(&s_LogLock);
        if (s_LogFile)
        {
            fflush(s_LogFile);
            fclose(s_LogFile);
            s_LogFile = nullptr;
        }
        ReleaseSRWLockExclusive(&s_LogLock);
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
#ifdef _DEBUG
        OutputDebugStringA(outBuf);

        AcquireSRWLockExclusive(&s_LogLock);
        if (s_LogFile)
        {
            fputs(outBuf, s_LogFile);
            fflush(s_LogFile);
        }
        ReleaseSRWLockExclusive(&s_LogLock);
#endif
    }
}
