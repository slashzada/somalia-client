#pragma once
#include <windows.h>
#include <cstdint>
#include <cstddef>

namespace SafeMemory
{
    // Faixa segura de memria de usurio para processos Win32 x86
    constexpr uintptr_t MIN_USER_ADDR = 0x00010000;
    constexpr uintptr_t MAX_USER_ADDR = 0x7FFE0000;

    inline bool IsValidUserAddress(const void* ptr, size_t size = 1)
    {
        if (!ptr || size == 0) return false;
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        if (addr < MIN_USER_ADDR || addr >= MAX_USER_ADDR) return false;
        if (size > (MAX_USER_ADDR - addr)) return false; // Impede qualquer overflow de adicao
        return true;
    }

    inline bool IsValidReadPtr(const void* ptr, size_t size)
    {
        if (!IsValidUserAddress(ptr, size)) return false;

        uintptr_t start = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t end = start + size;
        uintptr_t curr = start;

        while (curr < end)
        {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQuery(reinterpret_cast<LPCVOID>(curr), &mbi, sizeof(mbi)) != sizeof(mbi))
            {
                return false;
            }

            if (mbi.State != MEM_COMMIT)
            {
                return false;
            }

            if ((mbi.Protect & PAGE_GUARD) || (mbi.Protect & PAGE_NOACCESS))
            {
                return false;
            }

            DWORD readable = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
                             PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
            if ((mbi.Protect & readable) == 0)
            {
                return false;
            }

            uintptr_t next = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
            if (next <= curr) break;
            curr = next;
        }

        // Leitura de teste protegida por SEH em todas as paginas do intervalo
        __try
        {
            volatile const char* p = reinterpret_cast<volatile const char*>(ptr);
            for (size_t offset = 0; offset < size; offset += 4096)
            {
                volatile char dummy = p[offset];
                (void)dummy;
            }
            volatile char dummyEnd = p[size - 1];
            (void)dummyEnd;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    inline bool IsValidWritePtr(const void* ptr, size_t size)
    {
        if (!IsValidUserAddress(ptr, size)) return false;

        uintptr_t start = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t end = start + size;
        uintptr_t curr = start;

        while (curr < end)
        {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQuery(reinterpret_cast<LPCVOID>(curr), &mbi, sizeof(mbi)) != sizeof(mbi))
            {
                return false;
            }

            if (mbi.State != MEM_COMMIT)
            {
                return false;
            }

            if ((mbi.Protect & PAGE_GUARD) || (mbi.Protect & PAGE_NOACCESS))
            {
                return false;
            }

            DWORD writable = PAGE_READWRITE | PAGE_WRITECOPY |
                             PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
            if ((mbi.Protect & writable) == 0)
            {
                return false;
            }

            uintptr_t next = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
            if (next <= curr) break;
            curr = next;
        }

        // Teste de escrita SEH preservando o valor original em todas as paginas do intervalo
        __try
        {
            volatile char* p = reinterpret_cast<volatile char*>(const_cast<void*>(ptr));
            for (size_t offset = 0; offset < size; offset += 4096)
            {
                char original = p[offset];
                p[offset] = original;
            }
            char originalEnd = p[size - 1];
            p[size - 1] = originalEnd;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    inline bool SafeReadBuf(uintptr_t address, void* outBuf, size_t size)
    {
        if (!outBuf || size == 0) return false;
        if (!IsValidReadPtr(reinterpret_cast<const void*>(address), size))
        {
            return false;
        }

        __try
        {
            memcpy(outBuf, reinterpret_cast<const void*>(address), size);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    inline bool SafeWriteBuf(uintptr_t address, const void* inBuf, size_t size)
    {
        if (!inBuf || size == 0) return false;
        if (!IsValidWritePtr(reinterpret_cast<const void*>(address), size))
        {
            return false;
        }

        __try
        {
            memcpy(reinterpret_cast<void*>(address), inBuf, size);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    template<typename T>
    inline bool SafeRead(uintptr_t address, T& outVal)
    {
        return SafeReadBuf(address, &outVal, sizeof(T));
    }

    template<typename T>
    inline bool SafeWrite(uintptr_t address, const T& val)
    {
        return SafeWriteBuf(address, &val, sizeof(T));
    }
}
