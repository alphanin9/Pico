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
    // them in the WS. Keep in mind the main performance hog is the querying of the process working set, everything else takes very little time.

    const auto timestamp = std::chrono::high_resolution_clock::now();

    if (timestamp > m_nextWorkingSetCacheUpdate)
    {
        const auto start = std::chrono::high_resolution_clock::now();
        // Increment bad thread counter
        // Pray this doesn't race lol
        s_engine.m_threadsUnderHeavyLoad++;
        m_workingSetCache = shared::MemoryEnv::GetProcessWorkingSet();
        // Can wait for completion now, the rest doesn't take much
        s_engine.m_threadsUnderHeavyLoad--;
        const auto taken =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start)
                .count();

        logger->info("Time to capture working set: {}ms", taken);
        m_nextWorkingSetCacheUpdate = timestamp + WorkingSetUpdateTime;

        WalkWorkingSet();
        WalkSections();
    }
}

void pico::Engine::WorkingSetScanner::WalkWorkingSet() noexcept
{
    // Note: In CS2, 0x2000 will also catch trampolines made by GameOverlayRenderer64.dll
    // The maximum trampoline allocation size by overlay renderer seems to be 0x10000u
    // So, make it 0x10001u? No, less safe for small binaries
    constexpr auto MaxExecutableAllocationSize = 0x2000u;

    auto& logger = Logger::GetLogSink();

    for (auto workingSetEntry : m_workingSetCache)
    {
        // Transform the VPN into an actual address
        const auto pageAddr = reinterpret_cast<void*>(workingSetEntry.VirtualPage * 0x1000);

        // Skip non-executable pages and pages that belong to a section, we will scan sections later
        if ((workingSetEntry.Protection & (1u << 1u)) == 0u || workingSetEntry.Shared == 1)
        {
            continue;
        }

        MEMORY_BASIC_INFORMATION pageInfo{};

        // Might have deallocated or the like...
        if (!VirtualQuery(pageAddr, &pageInfo, sizeof(pageInfo)))
        {
            continue;
        }

        if (!shared::MemoryEnv::IsProtectionExecutable(pageInfo.Protect))
        {
            logger->warn("Executable in WS addr {} is no longer executable! Protection: {:#x}, alloc protect: {:#x}, "
                         "page type: {:#x}, size: {:#x}",
                         pageAddr, pageInfo.Protect, pageInfo.AllocationProtect, pageInfo.Type, pageInfo.RegionSize);
        }

        // MEM_IMAGE check might be useless here, not sure
        if (pageInfo.Type != MEM_IMAGE && pageInfo.RegionSize >= MaxExecutableAllocationSize)
        {
            logger->error("Executable addr {} is not image and is big! Protection: {:#x}, alloc protect: {:#x}, page "
                          "type: {:#x}, "
                          "size: {:#x}, base address: {}, state: {:#x}",
                          pageAddr, pageInfo.Protect, pageInfo.AllocationProtect, pageInfo.Type, pageInfo.RegionSize,
                          pageInfo.AllocationBase, pageInfo.State);
        }
    }
}

void pico::Engine::WorkingSetScanner::WalkSections() noexcept
{
    // TODO
}

pico::Engine::WorkingSetScanner& pico::Engine::WorkingSetScanner::Get() noexcept
{
    static WorkingSetScanner s_instance{};

    return s_instance;
}
