#include <Shared/SystemEnv/HandleEnumerator.hpp>

pico::Vector<Windows::SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX> pico::shared::SystemEnv::GetHandleTable()
{
    constexpr pico::Size DefaultSize = 0x4000;

    pico::Uint32 sizeNeeded{};

    pico::Vector<pico::Uint8> buffer(DefaultSize, {});

    auto status = Windows::NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS::SystemExtendedHandleInformation,
                                                    &buffer[0], (pico::Uint32)(buffer.size()), sizeNeeded);

    while (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        buffer.assign(sizeNeeded, {});

        status = Windows::NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS::SystemExtendedHandleInformation,
                                                   &buffer[0], (pico::Uint32)(buffer.size()), sizeNeeded);
    }

    if (!NT_SUCCESS(status))
    {
        return {};
    }

    Windows::SYSTEM_HANDLE_INFORMATION_EX* systemHandleInfo =
        (Windows::SYSTEM_HANDLE_INFORMATION_EX*)(buffer.data());

    pico::Vector<Windows::SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX> handleTable{};

    handleTable.reserve(systemHandleInfo->NumberOfHandles);

    for (auto i = 0u; i < systemHandleInfo->NumberOfHandles; i++)
    {
        handleTable.push_back(systemHandleInfo->Handles[i]);
    }

    return handleTable;
}
