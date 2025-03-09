#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>
#include <Shared/Util/NtdllCalls.hpp>

namespace pico::shared::SystemEnv
{
/**
 * \brief Queries processes running on the system.
 *
 * Fills a buffer pointed to by aBuffer with process information directly. In theory, this should cause
 * fewer allocations.
 *
 * \param aBuffer The buffer system process information will be placed into.
 *
 * \return Whether or not filling the buffer succeeded.
 */
pico::Bool FillSystemProcessInformationBuffer(pico::Vector<pico::Uint8>& aBuffer);

/**
 * \brief Enumerates system processes.
 *
 * \tparam Fn Function type. Specification is not required. Signature is
 * void(Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION*, const
 * pico::Vector<Windows::SYSTEM_EXTENDED_THREAD_INFORMATION*>&);
 *
 * \param aCallbackFunc Function to call upon process.
 */
template<typename Fn>
void EnumerateRunningProcesses(Fn&& aCallbackFunc)
{
    // Initialize a buffer for our output
    // A lot of Ntdll system calls and adjacent WinAPI interfaces will follow this pattern
    pico::Vector<pico::Uint8> buffer{};

    if (!pico::shared::SystemEnv::FillSystemProcessInformationBuffer(buffer))
    {
        return;
    }

    Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION* proc = (Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION*)(buffer.data());

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

        proc = (Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION*)((uintptr_t)(proc) + proc->NextEntryOffset);
    }
}
} // namespace pico::shared::SystemEnv