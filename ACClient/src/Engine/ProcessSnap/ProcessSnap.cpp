#include <Engine/Engine.hpp>
#include <Engine/Logging/Logger.hpp>
#include <Engine/ProcessSnap/ProcessSnap.hpp>

pico::Engine::ThreadInformation::ThreadInformation(Windows::SYSTEM_EXTENDED_THREAD_INFORMATION* aThread) noexcept
    : m_threadId(reinterpret_cast<pico::Uint32>(aThread->ThreadInfo.ClientId.UniqueThread))
    , m_userTime(aThread->ThreadInfo.UserTime.QuadPart)
    , m_kernelTime(aThread->ThreadInfo.KernelTime.QuadPart)
    , m_threadState(aThread->ThreadInfo.ThreadState)
    , m_waitReason(aThread->ThreadInfo.WaitReason)
    , m_tebAddr(reinterpret_cast<pico::Uint64>(aThread->TebBase))
    , m_startAddress(reinterpret_cast<pico::Uint64>(aThread->ThreadInfo.StartAddress))
    , m_win32StartAddress(reinterpret_cast<pico::Uint64>(aThread->Win32StartAddress))
{
}

pico::Engine::ProcessInformation::ProcessInformation(
    Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION* aProc,
    const pico::Vector<Windows::SYSTEM_EXTENDED_THREAD_INFORMATION*>& aThreads) noexcept
    : m_processId(reinterpret_cast<pico::Uint32>(aProc->UniqueProcessId))
    , m_sessionId(aProc->SessionId)
{
    if (aProc->ImageName.Buffer)
    {
        m_imageName = aProc->ImageName.Buffer;
        m_imageNameHash = shared::FNV1a64WideCharString(aProc->ImageName.Buffer);
    }

    m_threads.reserve(aThreads.size());

    for (auto thread : aThreads)
    {
        m_threads.emplace_back(thread);
    }
}

void pico::Engine::ProcessSnap::Tick() noexcept
{
    const auto now = Clock::now();

    if (now - m_lastProcessRefreshTime > ProcessCacheRefreshTime)
    {
        m_lastProcessRefreshTime = now;
        m_isDone = false;
        m_lastIndex = 0u;

        const auto start = Clock::now();

        EngineThreadLoadGuard guard{};

        m_processInfo.clear();

        // A little bit silly, I guess
        // We should be conserving the allocated buffer
        shared::SystemEnv::EnumerateRunningProcesses(
            [this](Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION* aProc,
                   const pico::Vector<Windows::SYSTEM_EXTENDED_THREAD_INFORMATION*>& aThreads)
            { m_processInfo.emplace_back(aProc, aThreads); });

        const auto taken = std::chrono::duration_cast<pico::Milliseconds>(Clock::now() - start).count();

        Logger::GetLogSink()->info("[ProcessSnap] Time building snapshot: {}ms", taken);
    }

    // This is currently a bit silly, but will get less silly with additional info gathering?

    if (m_isDone)
    {
        return;
    }

    constexpr auto MaxProcessesPerTick = 16;
    auto processesThisTick = 0;

    auto& logger = Logger::GetLogSink();

    for (; m_lastIndex < m_processInfo.size(); m_lastIndex++)
    {
        if (processesThisTick > MaxProcessesPerTick)
        {
            return;
        }

        // Note: we can look over the current process's threads here too!

        auto& proc = m_processInfo[m_lastIndex];
        logger->info("[ProcessSnap] ...");
        logger->info("[ProcessSnap] Process {} in session {}, thread count: {}", proc.m_processId, proc.m_sessionId,
                     proc.m_threads.size());

        if (!proc.m_imageName.empty())
        {
            logger->info("[ProcessSnap] Process name: {}, hash: {:#x}", shared::Util::ToUTF8(proc.m_imageName),
                         proc.m_imageNameHash);
        }
        else
        {
            logger->info("[ProcessSnap] No process name!");
        }

        for (auto& i : proc.m_threads)
        {
            logger->info("[ProcessSnap] Thread {}", i.m_threadId);
            logger->info("[ProcessSnap] Thread state: {}, wait reason: {}", static_cast<pico::Uint32>(i.m_threadState),
                         static_cast<pico::Uint32>(i.m_waitReason));

            logger->info("[ProcessSnap] Thread user time: {}, kernel time: {}", i.m_userTime, i.m_kernelTime);
            logger->info("[ProcessSnap] Thread TEB: {:#x}", i.m_tebAddr);
            logger->info("[ProcessSnap] Thread start addr: {:#x}, Win32 start addr: {:#x}", i.m_startAddress,
                         i.m_win32StartAddress);
        }

        logger->info("[ProcessSnap] ...");

        processesThisTick++;
    }

    m_isDone = true;
}

pico::Engine::ProcessSnap& pico::Engine::ProcessSnap::Get() noexcept
{
    static ProcessSnap s_instance{};

    return s_instance;
}
