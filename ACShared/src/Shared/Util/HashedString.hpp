#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>
#include <Shared/Util/Hashing.hpp>

namespace pico::shared
{
// Container for hash of lower case string
struct HashedString
{
    pico::Uint64 m_hash{};

    constexpr HashedString(const pico::Char* aStr) noexcept
        : m_hash(pico::shared::FNV1a64WithLowerCaseString(aStr))
    {
        
    }

    constexpr HashedString() noexcept
    {
    }

    constexpr pico::Uint64 Get() const noexcept
    {
        return m_hash;
    }

    constexpr operator const pico::Uint64() const noexcept
    {
        return Get();
    }

    constexpr pico::Bool operator==(pico::shared::HashedString aRhs) const noexcept
    {
        return m_hash == aRhs.m_hash;
    }

    constexpr pico::Bool operator!=(pico::shared::HashedString aRhs) const noexcept
    {
        return m_hash != aRhs.m_hash;
    }

    constexpr pico::Bool operator==(const pico::Char* aStr) const noexcept
    {
        return m_hash == pico::shared::FNV1a64WithLowerCaseString(aStr);
    }

    constexpr pico::Bool operator!=(const pico::Char* aStr) const noexcept
    {
        return !(*this == aStr);
    }
};
}