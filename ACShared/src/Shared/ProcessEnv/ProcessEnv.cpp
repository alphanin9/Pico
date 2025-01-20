#include <Shared/ProcessEnv/ProcessEnv.hpp>
#include <Shared/Util/StringUtils.hpp>

Windows::PEB* pico::shared::ProcessEnv::GetProcessEnvironment() noexcept
{
    static auto s_peb = reinterpret_cast<Windows::PEB*>(
        pico::shared::ProcessEnv::GetCurrentThreadEnvironment()->ProcessEnvironmentBlock);

    return s_peb;
}

Windows::TEB64* pico::shared::ProcessEnv::GetCurrentThreadEnvironment() noexcept
{
    return reinterpret_cast<Windows::TEB64*>(NtCurrentTeb());
}

pico::Vector<Windows::LDR_DATA_TABLE_ENTRY*> pico::shared::ProcessEnv::GetLoadedModuleList() noexcept
{
    // Note that this is not safe code
    // A module could unload while we're doing things and we'll all be screwed
    // Make a copy? Make lock class for NTDLL library loader mutex?

    pico::Vector<Windows::LDR_DATA_TABLE_ENTRY*> entries{};

    pico::shared::ProcessEnv::EnumerateLoadedModules(
        [&entries](Windows::LDR_DATA_TABLE_ENTRY* aEntry)
        {
            entries.push_back(aEntry);

            // Keep the loop going
            return false;
        });

    return entries;
}

Windows::LDR_DATA_TABLE_ENTRY* pico::shared::ProcessEnv::GetModuleByHash(pico::Uint64 aHash) noexcept
{
    Windows::LDR_DATA_TABLE_ENTRY* entry = nullptr;

    pico::shared::ProcessEnv::EnumerateLoadedModules(
        [&entry, aHash](Windows::LDR_DATA_TABLE_ENTRY* aEntry)
        {
            if (!aEntry->BaseDllName.Length)
            {
                return false;
            }

            if (aHash == pico::shared::FNV1a64WideCharStringWithLowerCase(aEntry->BaseDllName.Buffer))
            {
                entry = aEntry;
                // Break the loop
                return true;
            }

            return false;
        });

    return entry;
}

Windows::LDR_DATA_TABLE_ENTRY* pico::shared::ProcessEnv::GetModuleByName(pico::shared::HashedString aHash) noexcept
{
    return GetModuleByHash(aHash);
}

pico::Vector<Windows::SYSTEM_EXTENDED_THREAD_INFORMATION> pico::shared::ProcessEnv::GetCurrentProcessThreads() noexcept
{
    pico::Vector<Windows::SYSTEM_EXTENDED_THREAD_INFORMATION> threadInfo{};
    // Note: the HANDLE cast is actually correct, as SYSTEM_EXTENDED_PROCESS_INFORMATION::UniqueProcessId is actually
    // HANDLE

    static const auto s_pid =
        reinterpret_cast<HANDLE>(pico::shared::ProcessEnv::GetCurrentThreadEnvironment()->ClientId.UniqueProcess);
    shared::SystemEnv::EnumerateRunningProcesses(
        [&threadInfo](Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION* aProc,
                      const pico::Vector<Windows::SYSTEM_EXTENDED_THREAD_INFORMATION*>& aThreads)
        {
            if (aProc->UniqueProcessId == s_pid)
            {
                for (auto i : aThreads)
                {
                    threadInfo.push_back(*i);
                }
            }
        });

    return threadInfo;
}