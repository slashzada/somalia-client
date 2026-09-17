#pragma once
#include <string>
#include <array>
#include <utility>

namespace Obfuscation
{
    constexpr uint8_t CompileTimeKey(size_t index)
    {
        constexpr char time[] = __TIME__;
        uint8_t seed = static_cast<uint8_t>(time[7]) ^ static_cast<uint8_t>(time[6] << 2) ^ static_cast<uint8_t>(time[4] << 4);
        return static_cast<uint8_t>((seed + index * 0x5A) ^ 0xA5);
    }

    template <size_t N, size_t... Is>
    class XorString
    {
    private:
        std::array<char, N> m_Encrypted;

    public:
        constexpr XorString(const char(&str)[N])
            : m_Encrypted{ static_cast<char>(str[Is] ^ CompileTimeKey(Is))... }
        {
        }

        std::string decrypt() const
        {
            std::string result;
            result.resize(N - 1);
            for (size_t i = 0; i < N - 1; ++i)
            {
                result[i] = static_cast<char>(m_Encrypted[i] ^ CompileTimeKey(i));
            }
            return result;
        }

        const char* c_str() const
        {
            thread_local char decrypted[N];
            for (size_t i = 0; i < N; ++i)
            {
                decrypted[i] = static_cast<char>(m_Encrypted[i] ^ CompileTimeKey(i));
            }
            return decrypted;
        }
    };

    template <size_t N, size_t... Is>
    constexpr auto MakeXorHelper(const char(&str)[N], std::index_sequence<Is...>)
    {
        return XorString<N, Is...>(str);
    }

    template <size_t N>
    constexpr auto MakeXorString(const char(&str)[N])
    {
        return MakeXorHelper(str, std::make_index_sequence<N>{});
    }
}

#define XOR(str) (Obfuscation::MakeXorString(str).c_str())
#define XOR_STR(str) (Obfuscation::MakeXorString(str).decrypt())
