#include <Engine/WorkingSetWatch/WorkingSetWatch.hpp>
#include <Engine/Logging/Logger.hpp>

void pico::Engine::WorkingSetWatcher::Tick() noexcept
{
    if (!m_initialized)
    {
        // Set the process to use the working set watch
        m_initialized = true;
        Windows::NtSetInformationProcess(GetCurrentProcess(), Windows::PROCESSINFOCLASS::ProcessWorkingSetWatch,
                                         nullptr, 0u);
    }

    const auto start = Clock::now();

    pico::Uint32 sizeWritten{};

    // Note: we don't have to reset the buffer or anything beforehand, the function will give us endpoint anyway
    const auto status =
        Windows::NtQueryInformationProcess(GetCurrentProcess(), Windows::PROCESSINFOCLASS::ProcessWorkingSetWatchEx,
                                           m_workingSetWatchBuffer.data(), m_workingSetWatchBuffer.size() * sizeof(m_workingSetWatchBuffer[0]), sizeWritten);

    // Special case, means we don't have more PFs yet
    if (status == STATUS_NO_MORE_ENTRIES)
    {
        return;
    }

    // WTF? This should be limited to 2048 in kernel
    if (status == STATUS_BUFFER_TOO_SMALL || (!NT_SUCCESS(status)))
    {
        Logger::GetLogSink()->error("[WorkingSetWatch] PROCESSINFOCLASS::ProcessWorkingSetWatchEx call failed! Error reason: {:#x}", static_cast<pico::Uint32>(status));
        return;
    }

    // Note: look into making this a func...
    static const auto s_procId = static_cast<pico::Uint32>(shared::ProcessEnv::GetCurrentThreadEnvironment()->ClientId.UniqueProcess);

    auto& logger = Logger::GetLogSink();

    auto pageFaultsWalked = 0;

    // Note: currently this only detects external to process threads
    for (auto& entry : m_workingSetWatchBuffer)
    {
        // We found the end of the list, quit out of the loop
        if (entry.FaultingPc == nullptr)
        {
            break;
        }

        pageFaultsWalked++;

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

    logger->info("[WorkingSetWatch] Time taken to walk page faults: {}ms, faults walked: {}", timeTaken, pageFaultsWalked);
}

pico::Engine::WorkingSetWatcher& pico::Engine::WorkingSetWatcher::Get() noexcept
{
    static WorkingSetWatcher s_instance{};

    return s_instance;
}