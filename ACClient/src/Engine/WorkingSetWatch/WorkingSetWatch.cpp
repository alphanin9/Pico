#include <Engine/WorkingSetWatch/WorkingSetWatch.hpp>
#include <Engine/Logging/Logger.hpp>

void pico::Engine::WorkingSetWatcher::TickMainThread() noexcept
{
    if (!m_initialized)
    {
        // Set the process to use the working set watch
        m_initialized = true;
        Windows::NtSetInformationProcess(GetCurrentProcess(), Windows::PROCESSINFOCLASS::ProcessWorkingSetWatch,
                                         nullptr, 0u);
    }

    constexpr pico::Size MinimumBufferSize = 0x100;

    const auto start = Clock::now();

    // Note: kinda slow to be doing every frame! Maybe call it in the thread pool worker instead?
    pico::Vector<Windows::PROCESS_WS_WATCH_INFORMATION_EX> buffer(MinimumBufferSize, {});

    pico::Uint32 sizeWritten{};

    auto status =
        Windows::NtQueryInformationProcess(GetCurrentProcess(), Windows::PROCESSINFOCLASS::ProcessWorkingSetWatchEx,
                                           buffer.data(), buffer.size() * sizeof(buffer[0]), sizeWritten);

    while (status == STATUS_BUFFER_TOO_SMALL)
    {
        buffer.assign((sizeWritten / sizeof(buffer[0])) * 2u, {});

        status = Windows::NtQueryInformationProcess(GetCurrentProcess(), Windows::PROCESSINFOCLASS::ProcessWorkingSetWatchEx,
                                               buffer.data(), buffer.size() * sizeof(buffer[0]), sizeWritten);
    }

    std::unique_lock lock(m_workingSetWatchBufferLock);

    m_workingSetWatchBuffer = std::move(buffer);

    const auto timeTaken = std::chrono::duration_cast<pico::Milliseconds>(Clock::now() - start).count();

    if (timeTaken > 10)
    {
        Logger::GetLogSink()->error("[WorkingSetWatch] MAIN THREAD working set watcher took {}ms to gather entries!!! This is VERY BAD AND SHOULD NOT BE HAPPENING!!!!!!!!!!!!", timeTaken);
    }
}

void pico::Engine::WorkingSetWatcher::Tick() noexcept
{
    pico::Vector<Windows::PROCESS_WS_WATCH_INFORMATION_EX> buffer{};
    {
        std::unique_lock lock(m_workingSetWatchBufferLock);
        buffer = std::move(m_workingSetWatchBuffer);
    }

    // Note: look into making this a func...
    static const auto s_procId = static_cast<pico::Uint32>(shared::ProcessEnv::GetCurrentThreadEnvironment()->ClientId.UniqueProcess);

    auto& logger = Logger::GetLogSink();

    const auto start = Clock::now();

    // Note: currently this only detects external to process threads
    for (auto& entry : buffer)
    {
        // We found the end of the list, quit out of the loop
        if (entry.FaultingPc == nullptr)
        {
            break;
        }

        // Open a handle to the faulting thread
        wil::unique_handle threadHandle{OpenThread(THREAD_QUERY_LIMITED_INFORMATION, false, entry.FaultingThreadId)};

        if (!threadHandle)
        {
            // Fine, guess it's not OK
            continue;
        }

        const auto pid = GetProcessIdOfThread(threadHandle.get());

        if (pid != s_procId)
        {
            logger->warn("[WorkingSetWatch] Thread {} of process {} caused a page fault in us with RIP {} at addr {}",
                         entry.FaultingThreadId, pid, entry.FaultingPc, entry.FaultingVa);
        }
        else
        {
            void* image{};
            RtlPcToFileHeader(entry.FaultingPc, reinterpret_cast<PVOID*>(&image));

            // OK, this is weird...
            if (!image)
            {
                logger->warn("[WorkingSetWatch] Thread {} had bad RIP {} cause a page fault at addr {}", entry.FaultingThreadId,
                             entry.FaultingPc, entry.FaultingVa);
            }
        }
    }

    const auto timeTaken = std::chrono::duration_cast<pico::Milliseconds>(Clock::now() - start).count();

    logger->info("[WorkingSetWatch] Time taken to walk page faults: {}ms", timeTaken);
}

pico::Engine::WorkingSetWatcher& pico::Engine::WorkingSetWatcher::Get() noexcept
{
    static WorkingSetWatcher s_instance{};

    return s_instance;
}