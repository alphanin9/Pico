#include <Engine/Engine.hpp>
#include <Engine/Logging/Logger.hpp>
#include <Engine/WorkingSetScanner/WorkingSetScanner.hpp>

void pico::Engine::WorkingSetScanner::Tick() noexcept
{
    static auto& s_engine = Engine::Get();
    auto& logger = Logger::GetLogSink();

    // A potential optimization could be caching the addresses of the allocations we have already walked and skipping
    // them in the WS. Keep in mind the main performance hog is the querying of the process working set, everything else
    // takes very little time (maybe if you VirtualQuery everything in one tick).

    // Note: maybe using static in-class buffer for working set entries would be better?
    // 32MB should fit around 16GB of working set
    // 128MB should fit 64GB - plenty for most apps
    // Due to it being in .data it shouldn't meaningfully affect binary size

    const auto timestamp = Clock::now();

    if (timestamp > m_nextWorkingSetCacheUpdate)
    {
        shared::Util::MsTaken watch{};
        // Increment bad thread counter
        // Pray this doesn't race lol

        if (UpdateWorkingSet())
        {
            m_workingSetInfo = reinterpret_cast<Windows::MEMORY_WORKING_SET_INFORMATION*>(m_workingSetBuffer.data());

            logger->info("[WorkingSetScanner] Time to capture working set: {}ms", watch.Now());

            // Reset state
            m_lastIndex = 1u;
            m_doneWithScan = false;
        }

        m_nextWorkingSetCacheUpdate = timestamp + WorkingSetUpdateTime;        
    }

    if (!m_doneWithScan)
    {
        WalkWorkingSet();
    }
}

pico::Bool pico::Engine::WorkingSetScanner::UpdateWorkingSet() noexcept
{
    EngineThreadLoadGuard guard{};

    const auto currentProcess = GetCurrentProcess();

    pico::Uint32 sizeNeeded{};

    const auto status = Windows::NtQueryVirtualMemory(currentProcess, 0u, Windows::MEMORY_INFORMATION_CLASS::MemoryWorkingSetInformation,
                                  m_workingSetBuffer.data(), m_workingSetBuffer.size() * sizeof(m_workingSetBuffer[0]),
                                  sizeNeeded);

    if (!NT_SUCCESS(status))
    {
        Logger::GetLogSink()->error("[WorkingSetScanner] Failed to refresh working set buffer, status: {}",
                           static_cast<pico::Uint32>(status));

        return false;
    }

    return true;
}

void pico::Engine::WorkingSetScanner::WalkWorkingSet() noexcept
{
    auto& logger = Logger::GetLogSink();

    constexpr auto MaxVirtualQueryCallsPerPass = 15;

    auto queryCalls = 0;

    for (; m_lastIndex < m_workingSetInfo->NumberOfEntries; m_lastIndex++)
    {
        if (queryCalls >= MaxVirtualQueryCallsPerPass)
        {
            return;
        }

        const auto prevEntry = m_workingSetInfo->WorkingSetInfo[m_lastIndex - 1u];
        const auto curEntry = m_workingSetInfo->WorkingSetInfo[m_lastIndex];

        // Filter executable pages
        if ((curEntry.Protection & (1u << 1u)) == 0u)
        {
            continue;
        }

        // Filter contiguous pages (note: Will some executable pages be paged out? This can be false positive too!)
        if (std::abs(pico::Int64(prevEntry.VirtualPage) - pico::Int64(curEntry.VirtualPage)) <= 2 &&
            curEntry.Shared == prevEntry.Shared && curEntry.Protection == prevEntry.Protection)
        {
            continue;
        }

        // Transform VPN into actual virtual address for the page
        const auto pageAddr = reinterpret_cast<void*>(curEntry.VirtualPage * 0x1000);

        MEMORY_BASIC_INFORMATION pageInfo{};

        queryCalls++;

        // Might have deallocated or the like...
        if (!VirtualQuery(pageAddr, &pageInfo, sizeof(pageInfo)))
        {
            continue;
        }

        // Note: all non -executable pages are skipped anyway
        if (!shared::MemoryEnv::IsProtectionExecutable(pageInfo.Protect) ||
            !shared::MemoryEnv::IsProtectionExecutable(pageInfo.AllocationProtect))
        {
            logger->warn("[WorkingSetScanner] Execution status at page {} has been altered! Protection: {:#x}, alloc protect: {:#x}, "
                         "page type: {:#x}, base address: {}, size: {:#x}",
                         pageAddr, pageInfo.Protect, pageInfo.AllocationProtect, pageInfo.Type, pageInfo.BaseAddress,
                         pageInfo.RegionSize);
        }

        if (pageInfo.Type != MEM_IMAGE && pageInfo.RegionSize >= MaxExecutableAllocationSize)
        {
            logger->error("[WorkingSetScanner] Executable addr {} is not image and is big! Protection: {:#x}, alloc protect: {:#x}, page "
                          "type: {:#x}, "
                          "size: {:#x}, base address: {}, state: {:#x}",
                          pageAddr, pageInfo.Protect, pageInfo.AllocationProtect, pageInfo.Type, pageInfo.RegionSize,
                          pageInfo.BaseAddress, pageInfo.State);
        }
    }

    m_doneWithScan = true;
}

pico::Engine::WorkingSetScanner& pico::Engine::WorkingSetScanner::Get() noexcept
{
    static WorkingSetScanner s_instance{};

    return s_instance;
}
