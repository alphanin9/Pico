#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>
#include <Shared/Util/NtdllCalls.hpp>

namespace pico::shared::SystemEnv
{
/**
 * \brief Enumerates system processes.
 * 
 * \tparam Fn Function type. Specification is not required. Signature is void(Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION*, const pico::Vector<Windows::SYSTEM_EXTENDED_THREAD_INFORMATION*>&);
 * 
 * \param aCallbackFunc Function to call upon process. 
 */
template<typename Fn>
void EnumerateRunningProcesses(Fn&& aCallbackFunc)
{
    pico::Uint32 bufferSize = 0x1000;

    // Initialize a buffer for our output
    // A lot of Ntdll system calls and adjacent WinAPI interfaces will follow this pattern
    pico::Vector<pico::Uint8> buffer(bufferSize, pico::Uint8(0));

    auto status = Windows::NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS::SystemExtendedProcessInformation,
                                                    buffer.data(), bufferSize, bufferSize);

    // Resize it until we don't get error anymore
    while (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        buffer.resize(bufferSize);
        status = Windows::NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS::SystemExtendedProcessInformation,
                                                   buffer.data(), bufferSize, bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        // Exit early
        return;
    }

    Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION* proc =
        reinterpret_cast<Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION*>(buffer.data());

    // Iterate over all processes, breaking when NextEntryOffset == 0
    while (true)
    {
        pico::Vector<Windows::SYSTEM_EXTENDED_THREAD_INFORMATION*> threadList{};
        threadList.reserve(proc->NumberOfThreads);

        for (auto i = 0u; i < proc->NumberOfThreads; i++)
        {
            threadList.push_back(&proc->Threads[i]);
        }

        const auto& ref = threadList;

        aCallbackFunc(proc, ref);

        if (proc->NextEntryOffset == 0)
        {
            break;
        }

        proc = reinterpret_cast<Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION*>(reinterpret_cast<uintptr_t>(proc) +
                                                                               proc->NextEntryOffset);
    }
}
} // namespace pico::shared::SystemEnv