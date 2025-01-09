#include <Engine/Logging/Logger.hpp>
#include <Engine/WorkingSetScanner/WorkingSetScanner.hpp>

void pico::Engine::WorkingSetScanner::Tick() noexcept
{
    // Note: In CS2, this will also catch trampolines made by GameOverlayRenderer64.dll!!!
    constexpr auto MaxExecutableAllocationSize = 0x4000u;

    auto& logger = Logger::GetLogSink();

    // A potential optimization could be caching the addresses of the allocations we have already walked and skipping
    // them in the WS.

    for (auto workingSetEntry : shared::MemoryEnv::GetProcessWorkingSet())
    {
        // Transform the VPN into an actual address
        const auto pageAddr = reinterpret_cast<void*>(workingSetEntry.VirtualPage * 0x1000);

        // Skip non-executable pages
        if ((workingSetEntry.Protection & (1 << 1)) == 0)
        {
            continue;
        }

        MEMORY_BASIC_INFORMATION pageInfo{};

        if (!VirtualQuery(pageAddr, &pageInfo, sizeof(pageInfo)))
        {
            logger->warn("Executable addr {} had VirtualQuery call fail!", pageAddr);
            continue;
        }

        if (!shared::MemoryEnv::IsProtectionExecutable(pageInfo.Protect))
        {
            logger->warn("Executable in WS addr {} is no longer executable! Protection: {:#x}, alloc protect: {:#x}, "
                         "page type: {:#x}, size: {:#x}",
                         pageAddr, pageInfo.Protect, pageInfo.AllocationProtect, pageInfo.Type, pageInfo.RegionSize);
        }

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

pico::Engine::WorkingSetScanner& pico::Engine::WorkingSetScanner::Get() noexcept
{
    static WorkingSetScanner s_instance{};

    return s_instance;
}
