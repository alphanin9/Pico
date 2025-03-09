#include <Engine/Engine.hpp>
#include <Engine/Logging/Logger.hpp>
#include <Engine/ProcessSnap/ProcessSnap.hpp>

pico::Engine::ThreadInformation::ThreadInformation(Windows::SYSTEM_EXTENDED_THREAD_INFORMATION* aThread)
    : m_threadId((pico::Uint32)(aThread->ThreadInfo.ClientId.UniqueThread))
    , m_userTime(aThread->ThreadInfo.UserTime.QuadPart)
    , m_kernelTime(aThread->ThreadInfo.KernelTime.QuadPart)
    , m_threadState(aThread->ThreadInfo.ThreadState)
    , m_waitReason(aThread->ThreadInfo.WaitReason)
    , m_tebAddr((pico::Uint64)(aThread->TebBase))
    , m_startAddress((pico::Uint64)(aThread->ThreadInfo.StartAddress))
    , m_win32StartAddress((pico::Uint64)(aThread->Win32StartAddress))
{
}

pico::Engine::ProcessInformation::ProcessInformation(
    Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION* aProc,
    const pico::Vector<Windows::SYSTEM_EXTENDED_THREAD_INFORMATION*>& aThreads)
    : m_processId((pico::Uint32)(aProc->UniqueProcessId))
    , m_sessionId(aProc->SessionId)
{
    if (aProc->ImageName.Buffer)
    {
        m_imageName = aProc->ImageName.Buffer;
        m_imageNameHash = shared::FNV1a64WideCharString(aProc->ImageName.Buffer);
    }

    wil::unique_handle handle{OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, m_processId)};

    if (handle)
    {
        wil::QueryFullProcessImageNameW(handle.get(), 0u, m_imagePath);
    }

    m_threads.reserve(aThreads.size());

    for (auto thread : aThreads)
    {
        m_threads.emplace_back(thread);
    }
}

void pico::Engine::ProcessSnap::Tick()
{
    const auto now = Clock::now();

    if ((now - m_lastProcessRefreshTime) > ProcessCacheRefreshTime)
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

    constexpr auto MaxProcsPerTick = 16;
    auto procsThisTick = 0;

    auto& logger = Logger::GetLogSink();

    for (; m_lastIndex < m_processInfo.size(); m_lastIndex++)
    {
        if (procsThisTick > MaxProcsPerTick)
        {
            return;
        }

        // Note: we can look over the current process's threads here too!

        auto& proc = m_processInfo[m_lastIndex];
        logger->info("[ProcessSnap] Process {} (\"{}\" @ \"{}\", {}) in session {}, thread count: {}", proc.m_processId,
                     shared::Util::ToUTF8(proc.m_imageName), shared::Util::ToUTF8(proc.m_imagePath),
                     proc.m_imageNameHash, proc.m_sessionId, proc.m_threads.size());

        procsThisTick++;
    }

    m_isDone = true;
}

pico::Engine::ProcessSnap& pico::Engine::ProcessSnap::Get()
{
    static ProcessSnap s_instance{};

    return s_instance;
}
