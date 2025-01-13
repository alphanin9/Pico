#include <Engine/Engine.hpp>
#include <Engine/Logging/Logger.hpp>
#include <Engine/WorkingSetScanner/WorkingSetScanner.hpp>

void pico::Engine::WorkingSetScanner::Tick() noexcept
{
    // Time between working set updates, as querying the working set every frame is expensive
    constexpr std::chrono::seconds WorkingSetUpdateTime{5};

    static auto& s_engine = Engine::Get();
    auto& logger = Logger::GetLogSink();

    // A potential optimization could be caching the addresses of the allocations we have already walked and skipping
    // them in the WS. Keep in mind the main performance hog is the querying of the process working set, everything else
    // takes very little time.

    const auto timestamp = std::chrono::high_resolution_clock::now();

    if (timestamp > m_nextWorkingSetCacheUpdate)
    {
        const auto start = std::chrono::high_resolution_clock::now();
        // Increment bad thread counter
        // Pray this doesn't race lol
        s_engine.m_threadsUnderHeavyLoad++;
        m_workingSetCache = shared::MemoryEnv::GetProcessWorkingSet();
        // m_scannedPages.clear();
        // Can wait for completion now, the rest doesn't take much
        s_engine.m_threadsUnderHeavyLoad--;
        const auto taken =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start)
                .count();

        logger->info("Time to capture working set: {}ms", taken);
        m_nextWorkingSetCacheUpdate = timestamp + WorkingSetUpdateTime;

        WalkWorkingSet();
    }
}

void pico::Engine::WorkingSetScanner::ProcessPage(Windows::MEMORY_WORKING_SET_BLOCK aBlock) noexcept
{
    // Note: figure out how to scan shared pages in a more optimized fashion!
    // Maybe abandon working set completely and just walk VirtualQuery results?

    auto& logger = Logger::GetLogSink();

    // Note: hack for lazier mappers (check for shared section permissions being RWX), still should optimize it
    // I mean, what if there's some legit image with RWX image sections that triggers lots of VirtualQuery calls and
    // lags the app to a standstill?
    if (aBlock.Shared == 1 && (aBlock.Protection & (1u << 2u)) == 0u)
    {
        return;
    }

    // Transform VPN into actual virtual address for the page
    const auto pageAddr = reinterpret_cast<void*>(aBlock.VirtualPage * 0x1000);

    MEMORY_BASIC_INFORMATION pageInfo{};

    // Might have deallocated or the like...
    if (!VirtualQuery(pageAddr, &pageInfo, sizeof(pageInfo)))
    {
        return;
    }

    // Note: all non-executable pages are skipped anyway
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

    // Math should be OK
    // const auto basePage = reinterpret_cast<pico::Uint64>(pageInfo.BaseAddress) / 0x1000u;
    // const auto finalPage = basePage + (pageInfo.RegionSize / 0x1000u);

    // for (auto i = basePage; i <= finalPage; i++)
    //{
    //     m_scannedPages.insert(i);
    // }
}

void pico::Engine::WorkingSetScanner::WalkWorkingSet() noexcept
{
    auto& logger = Logger::GetLogSink();

    const auto start = std::chrono::high_resolution_clock::now();

    for (auto workingSetEntry : m_workingSetCache)
    {
        // Skip non-executable pages
        if ((workingSetEntry.Protection & (1u << 1u)) == 0u)
        {
            continue;
        }

        ProcessPage(workingSetEntry);
    }

    const auto taken =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start)
            .count();

    logger->info("Time to walk working set: {}ms", taken);
}

pico::Engine::WorkingSetScanner& pico::Engine::WorkingSetScanner::Get() noexcept
{
    static WorkingSetScanner s_instance{};

    return s_instance;
}
