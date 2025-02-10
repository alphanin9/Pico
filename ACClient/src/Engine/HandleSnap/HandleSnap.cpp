#include <Engine/Engine.hpp>
#include <Engine/HandleSnap/HandleSnap.hpp>
#include <Engine/Logging/Logger.hpp>

void pico::Engine::HandleSnap::Tick()
{
    static const auto s_localPid =
        static_cast<pico::Uint32>(shared::ProcessEnv::GetCurrentThreadEnvironment()->ClientId.UniqueProcess);

    constexpr pico::Seconds CheckInterval{5};
    const auto now = Clock::now();

    if (now - m_lastCheckTime > CheckInterval)
    {
        // Make sure no stutters happen
        EngineThreadLoadGuard guard{};

        m_isDone = false;
        m_lastHandleIndex = 0u;
        m_lastCheckTime = now;

        const auto start = Clock::now();

        m_handleEntryCache = shared::SystemEnv::GetHandleTable();

        const auto duration = std::chrono::duration_cast<pico::Milliseconds>(Clock::now() - start).count();

        Logger::GetLogSink()->info("[HandleSnap] Time taken to cache handle table: {}ms", duration);
    }

    if (m_isDone)
    {
        return;
    }

    // Keep overall load low, we utilize a similar pattern as with the process working set scanner to spread the load
    // around calls
    constexpr auto MaxCheckedHandlesPerTick = 64;

    // The amount of process handles we managed to open this time around
    auto checkedHandles = 0;

    constexpr pico::Uint32 SystemIdleProcId = 0u;
    constexpr pico::Uint32 SystemUniqueProcessId = 4u;

    for (auto i = m_lastHandleIndex; i < m_handleEntryCache.size(); i++)
    {
        if (checkedHandles > MaxCheckedHandlesPerTick)
        {
            m_lastHandleIndex = i;
            return;
        }

        auto& entry = m_handleEntryCache[i];

        const auto pid = reinterpret_cast<pico::Uint32>(entry.UniqueProcessId);

        if (pid == SystemIdleProcId || pid == SystemUniqueProcessId || pid == s_localPid)
        {
            continue;
        }

        constexpr auto ObjectTypeProcess = 8u;
        constexpr auto ObjectTypeThread = 9u;

        const auto isProcess = entry.ObjectTypeIndex == ObjectTypeProcess;
        const auto isThread = entry.ObjectTypeIndex == ObjectTypeThread;

        if (!isProcess && !isThread)
        {
            continue;
        }

        wil::unique_handle ownerHandle{OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, false, pid)};

        if (!ownerHandle.is_valid())
        {
            continue;
        }

        checkedHandles++;

        if (isProcess)
        {
            OnProcessHandleCheck(entry, ownerHandle);
        }
        else
        {
            OnThreadHandleCheck(entry, ownerHandle);
        }
    }

    m_isDone = true;
}

void pico::Engine::HandleSnap::OnProcessHandleCheck(const Windows::SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX& aEntry,
                                                    const wil::unique_handle& aOwner)
{
    constexpr auto CheckedMask = PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE;

    static const auto s_localPid =
        static_cast<pico::Uint32>(shared::ProcessEnv::GetCurrentThreadEnvironment()->ClientId.UniqueProcess);
    static const auto s_currentProcess = GetCurrentProcess();

    if ((aEntry.GrantedAccess & CheckedMask) == 0u)
    {
        // The entry doesn't matter in all likelihood
        return;
    }

    auto& logger = Logger::GetLogSink();

    wil::unique_handle dupHandle{};

    const auto pid = reinterpret_cast<pico::Uint32>(aEntry.UniqueProcessId);

    if (!NT_SUCCESS(Windows::NtDuplicateObject(aOwner.get(), aEntry.HandleValue, s_currentProcess,
                                               *dupHandle.addressof(), PROCESS_QUERY_LIMITED_INFORMATION, 0u, 0u)))
    {
        logger->warn("[HandleSnap] Failed to duplicate process handle from PID {}", pid);
        return;
    }

    const auto targetPid = GetProcessId(dupHandle.get());

    if (targetPid == s_localPid)
    {
        pico::UnicodeString procName{};
        wil::QueryFullProcessImageNameW(aOwner.get(), 0u, procName);

        logger->warn(
            "[HandleSnap] Process {} (PID {}) has a potentially dangerous process handle ({}, access: {}) open to us!",
            shared::Util::ToUTF8(procName), pid, aEntry.HandleValue, aEntry.GrantedAccess);
    }
}

void pico::Engine::HandleSnap::OnThreadHandleCheck(const Windows::SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX& aEntry,
                                                   const wil::unique_handle& aOwner)
{
    constexpr auto CheckedMask = THREAD_TERMINATE | THREAD_SUSPEND_RESUME | THREAD_SET_CONTEXT | THREAD_GET_CONTEXT;

    static const auto s_localPid =
        static_cast<pico::Uint32>(shared::ProcessEnv::GetCurrentThreadEnvironment()->ClientId.UniqueProcess);
    static const auto s_currentProcess = GetCurrentProcess();

    if ((aEntry.GrantedAccess & CheckedMask) == 0u)
    {
        return;
    }

    auto& logger = Logger::GetLogSink();

    wil::unique_handle dupHandle{};

    const auto pid = reinterpret_cast<pico::Uint32>(aEntry.UniqueProcessId);

    if (!NT_SUCCESS(Windows::NtDuplicateObject(aOwner.get(), aEntry.HandleValue, s_currentProcess,
                                               *dupHandle.addressof(), THREAD_QUERY_LIMITED_INFORMATION, 0u, 0u)))
    {
        logger->warn("[HandleSnap] Failed to duplicate thread handle from PID {}", pid);
        return;
    }

    const auto targetPid = GetProcessIdOfThread(dupHandle.get());

    if (targetPid == s_localPid)
    {
        pico::UnicodeString procName{};
        wil::QueryFullProcessImageNameW(aOwner.get(), 0u, procName);

        logger->warn(
            "[HandleSnap] Process {} (PID {}) has a potentially dangerous thread handle ({}, access: {}) open to us!",
            shared::Util::ToUTF8(procName), pid, aEntry.HandleValue, aEntry.GrantedAccess);
    }
}

pico::Engine::HandleSnap& pico::Engine::HandleSnap::Get()
{
    static HandleSnap s_instance{};

    return s_instance;
}
