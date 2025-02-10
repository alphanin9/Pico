#include <Engine/InstrumentationCallbacks/InstrumentationCallbacks.hpp>
#include <Engine/Logging/Logger.hpp>
#include <Engine/WorkingSetWatch/WorkingSetWatch.hpp>

void pico::Engine::WorkingSetWatcher::Tick() noexcept
{
    if (!m_initialized)
    {
        // Set the process to use the working set watch
        m_initialized = true;
        Windows::NtSetInformationProcess(GetCurrentProcess(), Windows::PROCESSINFOCLASS::ProcessWorkingSetWatch,
                                         nullptr, 0u);
    }

    pico::Uint32 sizeWritten{};

    // Note: we don't have to reset the buffer or anything beforehand, the function will give us endpoint anyway
    const auto status = Windows::NtQueryInformationProcess(
        GetCurrentProcess(), Windows::PROCESSINFOCLASS::ProcessWorkingSetWatchEx, m_workingSetWatchBuffer.data(),
        m_workingSetWatchBuffer.size() * sizeof(m_workingSetWatchBuffer[0]), sizeWritten);

    // Special case, means we don't have more PFs yet
    if (status == STATUS_NO_MORE_ENTRIES)
    {
        return;
    }

    // WTF? This should be limited to 2048 in kernel
    if (status == STATUS_BUFFER_TOO_SMALL || (!NT_SUCCESS(status)))
    {
        Logger::GetLogSink()->error(
            "[WorkingSetWatch] PROCESSINFOCLASS::ProcessWorkingSetWatchEx call failed! Error reason: {:#x}",
            static_cast<pico::Uint32>(status));
        return;
    }

    // This should always be the correct number
    const auto faultCount = (sizeWritten / sizeof(Windows::PROCESS_WS_WATCH_INFORMATION_EX)) - 1u;

    auto& logger = Logger::GetLogSink();

    static const auto& s_instrumentationCallbacks = InstrumentationCallbacks::Get();

    // A note: the old thread check was pointless, as nt!PspQueryWorkingSetWatch will filter out any page faults with
    // kernel mode addresses - and RPM/WPM will incur page faults in kernel mode for obvious reasons
    for (auto i = 0u; i < faultCount; i++)
    {
        auto& entry = m_workingSetWatchBuffer[i];

        void* image{};
        RtlPcToFileHeader(entry.FaultingPc, reinterpret_cast<PVOID*>(&image));

        // OK, this is weird...
        if (!image &&
            !s_instrumentationCallbacks.IsCodeInInstrumentationCallback(reinterpret_cast<uintptr_t>(entry.FaultingPc)))
        {
            logger->warn("[WorkingSetWatch] Thread {} had bad RIP {} cause a page fault at addr {}",
                         entry.FaultingThreadId, entry.FaultingPc, entry.FaultingVa);
        }
    }
}

pico::Engine::WorkingSetWatcher& pico::Engine::WorkingSetWatcher::Get() noexcept
{
    static WorkingSetWatcher s_instance{};

    return s_instance;
}