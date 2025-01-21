#include <Engine/Engine.hpp>
#include <Engine/Logging/Logger.hpp>
#include <Engine/WorkingSetScanner/WorkingSetScanner.hpp>

void pico::Engine::WorkingSetScanner::Tick() noexcept
{
    static auto& s_engine = Engine::Get();
    auto& logger = Logger::GetLogSink();

    // A potential optimization could be caching the addresses of the allocations we have already walked and skipping
    // them in the WS. Keep in mind the main performance hog is the querying of the process working set, everything else
    // takes very little time.

    const auto timestamp = Clock::now();

    if (timestamp > m_nextWorkingSetCacheUpdate)
    {
        const auto start = Clock::now();
        // Increment bad thread counter
        // Pray this doesn't race lol
        {
            EngineThreadLoadGuard guard{};
            m_workingSetCache = shared::MemoryEnv::GetProcessWorkingSet();
            // Can wait for completion now, the rest doesn't take much
        }
        
        const auto taken =
            std::chrono::duration_cast<pico::Milliseconds>(Clock::now() - start)
                .count();

        logger->info("Time to capture working set: {}ms", taken);
        m_nextWorkingSetCacheUpdate = timestamp + WorkingSetUpdateTime;

        // Reset state
        m_lastIndex = 1u;
        m_doneWithScan = false;
    }

    if (!m_doneWithScan)
    {
        WalkWorkingSet();
    }
}

void pico::Engine::WorkingSetScanner::WalkWorkingSet() noexcept
{
    auto& logger = Logger::GetLogSink();

    const auto start = Clock::now();

    constexpr auto MaxVirtualQueryCallsPerPass = 15;

    auto queryCalls = 0;

    for (pico::Size i = m_lastIndex; i < m_workingSetCache.size(); i++)
    {
        const auto prevEntry = m_workingSetCache[i - 1u];
        const auto curEntry = m_workingSetCache[i];

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
        if (queryCalls >= MaxVirtualQueryCallsPerPass)
        {
            m_lastIndex = i;
            return;
        }

        // Might have deallocated or the like...
        if (!VirtualQuery(pageAddr, &pageInfo, sizeof(pageInfo)))
        {
            continue;
        }

        // Note: all non -executable pages are skipped anyway
        if (!shared::MemoryEnv::IsProtectionExecutable(pageInfo.Protect) ||
            !shared::MemoryEnv::IsProtectionExecutable(pageInfo.AllocationProtect))
        {
            logger->warn("Execution status at page {} has been altered! Protection: {:#x}, alloc protect: {:#x}, "
                         "page type: {:#x}, base address: {}, size: {:#x}",
                         pageAddr, pageInfo.Protect, pageInfo.AllocationProtect, pageInfo.Type, pageInfo.BaseAddress,
                         pageInfo.RegionSize);
        }

        if (pageInfo.Type != MEM_IMAGE && pageInfo.RegionSize >= MaxExecutableAllocationSize)
        {
            logger->error("Executable addr {} is not image and is big! Protection: {:#x}, alloc protect: {:#x}, page "
                          "type: {:#x}, "
                          "size: {:#x}, base address: {}, state: {:#x}",
                          pageAddr, pageInfo.Protect, pageInfo.AllocationProtect, pageInfo.Type, pageInfo.RegionSize,
                          pageInfo.BaseAddress, pageInfo.State);
        }
    }

    m_doneWithScan = true;

    const auto taken =
        std::chrono::duration_cast<pico::Milliseconds>(Clock::now() - start)
            .count();

    logger->info("Time to walk working set: {}ms", taken);
}

pico::Engine::WorkingSetScanner& pico::Engine::WorkingSetScanner::Get() noexcept
{
    static WorkingSetScanner s_instance{};

    return s_instance;
}
