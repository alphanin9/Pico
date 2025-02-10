#pragma once
#include <Shared/StdlibTypeDefinitions.hpp>
namespace pico::shared
{
// Simple compile-time string class
template<pico::Size N>
struct StaticString
{
    static constexpr pico::Size Size = N;
    static constexpr pico::Size NullTerminatedSize = N - 1;

    pico::Char m_buffer[N];

    constexpr StaticString(const pico::Char (&aStr)[N])
    {
        std::copy_n(aStr, N, m_buffer);
    }

    constexpr const pico::Char* Get() const
    {
        return reinterpret_cast<const pico::Char*>(&m_buffer[0]);
    }

    constexpr operator const pico::Char*() const
    {
        return Get();
    }
};
} // namespace pico::shared