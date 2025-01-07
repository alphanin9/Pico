#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>
#include <Shared/Util/NtdllCalls.hpp>

namespace pico::shared::SystemEnv
{
/**
 * \brief Enumerates opened handles in the system and calls aCallback on them.
 * \tparam Fn Type of callback function. Function signature should be void(const Windows::SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX&);
 * \param aCallback Callback function.
 */
template<typename Fn>
inline void EnumerateHandles(Fn&& aCallback)
{
    constexpr pico::Size DefaultSize = 0x1000;

    pico::Uint32 sizeNeeded{};

    pico::Vector<pico::Uint8> buffer(DefaultSize, {});

    auto status = Windows::NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS::SystemExtendedHandleInformation,
                                                    &buffer[0], buffer.size(), sizeNeeded);

    while (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        buffer.assign(sizeNeeded, {});

        status = Windows::NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS::SystemExtendedHandleInformation,
                                                   &buffer[0], buffer.size(), sizeNeeded);
    }

    if (!NT_SUCCESS(status))
    {
        return;
    }

    Windows::SYSTEM_HANDLE_INFORMATION_EX* systemHandleInfo =
        reinterpret_cast<Windows::SYSTEM_HANDLE_INFORMATION_EX*>(buffer.data());

    for (auto i = 0u; i < systemHandleInfo->NumberOfHandles; i++)
    {
        const auto& entry = systemHandleInfo->Handles[i];

        aCallback(entry);
    }
}
} // namespace pico::shared::SystemEnv
