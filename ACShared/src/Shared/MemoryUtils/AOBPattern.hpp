#pragma once
#include <Shared/Common.hpp>
#include <Shared/PE/PE.hpp>
#include <Shared/ProcessEnv/ProcessEnv.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>
#include <Shared/Util/HashedString.hpp>
#include <Shared/Util/StaticString.hpp>

namespace pico::shared
{
/**
 * \brief Simple class to make raw ptr / func types support AOB patterns.
 *
 * AOB pattern format is code style, with \xCC as wildcard in pattern
 * This minimizes need for constexpr processing of pattern
 * CC wildcard is not optimal and you should probably use 2A, but CC is more memorable
 * NOTE: the fact we technically should be using Uint8 array instead of char doesn't really matter
 */
template<pico::shared::HashedString ModuleName, pico::shared::StaticString Pattern>
struct AOBPattern
{
    inline static uintptr_t s_resolvedAddr{};

    inline static uintptr_t Resolve()
    {
        if (s_resolvedAddr)
        {
            return s_resolvedAddr;
        }

        const auto moduleData = pico::shared::ProcessEnv::GetModuleByName(ModuleName);

        // ...is the module present?
        if (!moduleData)
        {
            return 0u;
        }

        auto pe = pico::shared::PE::GetImagePtr(moduleData->DllBase);

        if (!pe)
        {
            return 0u;
        }

        // TODO: add support for only scanning one section
        // I don't think we need to scan for PE ever
        auto [scanningMin, scanningMax] = pico::shared::PE::GetImageBounds(pe);

        if (!scanningMin || !scanningMax)
        {
            return 0u;
        }

        // Kinda evil, but what can you do?
        const auto stringBuffer = reinterpret_cast<const pico::Uint8*>(Pattern.Get());
        const auto stringBufferEnd = stringBuffer + (Pattern.NullTerminatedSize);

        // Find AOB in haystack (aka module bounds)
        const auto iter = std::search(
            reinterpret_cast<pico::Uint8*>(scanningMin), reinterpret_cast<pico::Uint8*>(scanningMax), stringBuffer,
            stringBufferEnd, [](pico::Uint8 aLeft, pico::Uint8 aRight) { return aLeft == aRight || aRight == 0xCC; });

        const auto asUintptr = reinterpret_cast<uintptr_t>(iter);

        if (asUintptr == scanningMax)
        {
            return 0u;
        }

        s_resolvedAddr = asUintptr;
        return s_resolvedAddr;
    }

    constexpr AOBPattern()
    {
    }
};
} // namespace pico::shared