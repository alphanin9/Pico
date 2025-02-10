#pragma once
#include <Shared/StdlibTypeDefinitions.hpp>

namespace pico::shared
{
inline constexpr pico::Uint64 FNV1a64(const pico::Uint8* aData, pico::Size aSize)
{
    constexpr pico::Uint64 Seed = 0xCBF29CE484222325;
    constexpr pico::Uint64 Prime = 0x100000001b3;

    auto hash = Seed;

    for (pico::Size i = 0u; i != aSize; i++)
    {
        hash ^= aData[i];
        hash *= Prime;
    }

    return 0;
}

inline constexpr pico::Uint64 FNV1a64(const pico::Char* aStr)
{
    constexpr pico::Uint64 Seed = 0xCBF29CE484222325;
    constexpr pico::Uint64 Prime = 0x100000001b3;

    auto hash = Seed;

    while (*aStr)
    {
        hash ^= *aStr;
        hash *= Prime;

        aStr++;
    }

    return hash;
}
// Hashes a lowercase version of the string
inline constexpr pico::Uint64 FNV1a64WithLowerCaseString(const pico::Char* aStr)
{
    constexpr pico::Uint64 Seed = 0xCBF29CE484222325;
    constexpr pico::Uint64 Prime = 0x100000001b3;

    auto hash = Seed;

    while (*aStr)
    {
        hash ^= ((*aStr >= 'A' && *aStr <= 'Z') ? *aStr + ('a' - 'A') : *aStr);
        hash *= Prime;

        aStr++;
    }

    return hash;
}

// Hashes wchar_t* ANSI strings so they are comparable to regular strings
inline constexpr pico::Uint64 FNV1a64WideCharString(const pico::WChar* aStr)
{
    constexpr pico::Uint64 Seed = 0xCBF29CE484222325;
    constexpr pico::Uint64 Prime = 0x100000001b3;

    auto hash = Seed;

    while (*aStr)
    {
        // Get lower byte of wchar_t
        auto hashedChar = *aStr & 0xFF;

        hash ^= hashedChar;
        hash *= Prime;

        aStr++;
    }

    return hash;
}

// Hashes wchar_t* ANSI strings so they are comparable to regular strings
// Turns them to lower case
inline constexpr pico::Uint64 FNV1a64WideCharStringWithLowerCase(const pico::WChar* aStr)
{
    constexpr pico::Uint64 Seed = 0xCBF29CE484222325;
    constexpr pico::Uint64 Prime = 0x100000001b3;

    auto hash = Seed;

    while (*aStr)
    {
        // Get lower byte of wchar_t
        auto hashedChar = *aStr & 0xFF;

        hash ^= ((hashedChar >= 'A' && hashedChar <= 'Z') ? hashedChar + ('a' - 'A') : hashedChar);
        hash *= Prime;

        aStr++;
    }

    return hash;
}

inline constexpr pico::Uint64 FNV1a64WideCharStringWithLowerCase(const pico::WChar* aStr, pico::Size aSize)
{
    constexpr pico::Uint64 Seed = 0xCBF29CE484222325;
    constexpr pico::Uint64 Prime = 0x100000001b3;

    auto hash = Seed;

    for (auto i = 0u; i < aSize; i++)
    {
        // Get lower byte of WChar
        auto hashedChar = *aStr & 0xFF;

        hash ^= ((hashedChar >= 'A' && hashedChar <= 'Z') ? hashedChar + ('a' - 'A') : hashedChar);
        hash *= Prime;

        aStr++;
    }

    return hash;
}
}; // namespace pico::shared