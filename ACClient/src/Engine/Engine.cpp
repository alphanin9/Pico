#include <Engine/ContextScanner/ContextScanner.hpp>
#include <Engine/Engine.hpp>
#include <Engine/HandleSnap/HandleSnap.hpp>
#include <Engine/IntegrityChecker/IntegrityChecker.hpp>
#include <Engine/Logging/Logger.hpp>
#include <Engine/ProcessSnap/ProcessSnap.hpp>
#include <Engine/ThreadPool/ThreadPool.hpp>
#include <Engine/WorkingSetScanner/WorkingSetScanner.hpp>
#include <Engine/WorkingSetWatch/WorkingSetWatch.hpp>

#include <DevIntegration/Integration.hpp>

#include <Engine/Specific/CS2/CS2.hpp>
void pico::Engine::Engine::Tick() noexcept
{
    TickMainThreadJobs();

    auto& threadPool = ThreadPool::Get();

    if (!threadPool.CanPushJobs())
    {
        // Don't add new jobs yet...
        return;
    }

    // Dispatch jobs here.
    threadPool.Dispatch([]() { ContextScanner::Get().Tick(); });
    threadPool.Dispatch([]() { IntegrityChecker::Get().Tick(); });
    threadPool.Dispatch([]() { WorkingSetScanner::Get().Tick(); });
    threadPool.Dispatch([]() { HandleSnap::Get().Tick(); });
    //threadPool.Dispatch([]() { ProcessSnap::Get().Tick(); });
    threadPool.Dispatch([]() { WorkingSetWatcher::Get().Tick(); });
    threadPool.Dispatch([]() { Logger::Get().Tick(); });

    static const auto s_isCS2 = Integration::IsCS2();

    if (s_isCS2)
    {
        threadPool.Dispatch([]() { Specific::CS2::Get().Tick(); });
    }
}

void pico::Engine::Engine::TickMainThreadJobs() noexcept
{
    // Modulos for timestamp counter to check things
    constexpr auto ThreadPoolCheck = 0x51Cu;
    constexpr auto ThreadCtxCheck = 0xC15u;

    const auto timestampCounter = __rdtsc();

    auto& logger = Logger::Get();

    if ((timestampCounter % ThreadPoolCheck) == 0 && !IsThreadPoolOK())
    {
        // In production we should report the issue and crash
        logger.m_logger->error("[Engine] Thread pool had issues!");
    }

    if ((timestampCounter % ThreadCtxCheck) == 0)
    {
        ContextScanner::Get().TickMainThread();
    }
}

pico::Bool pico::Engine::Engine::IsThreadPoolOK() noexcept
{
    static auto& threadPool = ThreadPool::Get();
    auto& logger = Logger::GetLogSink();

    // Dispatch job to all threads
    // NOTE: on second thoughts this is a bit silly,
    // but it should still catch suspended threads every now and then

    if (m_threadsUnderHeavyLoad < 0)
    {
        logger->critical("[Engine] Engine::m_threadsUnderHeavyLoad is negative, this should NEVER happen!");
    }

    // While this is not perfect at all, this check should help out with stutters a bit
    if (m_threadsUnderHeavyLoad <= 0)
    {
        std::atomic<pico::Int64> counter{};
        for (auto i = 0u; i < threadPool.m_pool.get_thread_count(); i++)
        {
            threadPool.Dispatch([&counter]() { counter++; }, BS::pr::highest);
        }

        // Given this is called before threadPool.CanPushJobs(),
        // this may also end up waiting on normal worker threads - good
        // Not actually very good, given we can have long-running jobs here!

        const auto start = Clock::now();

        threadPool.m_pool.wait();

        const auto duration = std::chrono::duration_cast<pico::Milliseconds>(Clock::now() - start).count();

        logger->info("[Engine] Time taken to wait: {}ms", duration);

        if (duration > 20)
        {
            logger->error("[Engine] Unacceptable stutter from pool wait! This is a PROBLEM.");
        }

        if (threadPool.m_pool.get_thread_count() != counter)
        {
            logger->error("[Engine] Not all threads responded to pool");
            return false;
        }
    }

    for (HANDLE i : threadPool.m_pool.get_native_handles())
    {
        Windows::SYSTEM_THREAD_INFORMATION threadInfo{};

        pico::Uint32 sizeWritten{};

        if (!NT_SUCCESS(Windows::NtQueryInformationThread(i, Windows::THREADINFOCLASS::ThreadSystemThreadInformation,
                                                          &threadInfo, sizeof(threadInfo), sizeWritten)))
        {
            logger->error("[Engine] Failed to call NtQueryInformationThread!");
            return false;
        }

        if (threadInfo.WaitReason == Windows::KWAIT_REASON::Suspended ||
            threadInfo.WaitReason == Windows::KWAIT_REASON::WrSuspended)
        {
            logger->error("[Engine] Found suspended thread in thread pool!");
            return false;
        }
    }

    return true;
}

void pico::Engine::Engine::Setup() noexcept
{
    if (m_hasRunPreflight)
    {
        return;
    }

    SetupModuleData();

    auto& logger = Logger::GetLogSink();

    logger->info("[Preflight] Pico engine preflight started");

    const auto secureBootConfig = shared::EnvironmentIntegrity::GetSecureBootConfig();

    logger->info("[Preflight] Secure Boot config: can use: {}, is on: {}", secureBootConfig.m_secureBootSupported,
                 secureBootConfig.m_secureBootEnabled);

    if (!secureBootConfig.m_secureBootEnabled)
    {
        // Should notify user and not execute
        logger->error("[Preflight] Secure Boot is NOT enabled");
    }

    // Note: add Measured Boot log dump here

    const auto codeIntegrityConfig = shared::EnvironmentIntegrity::GetCodeIntegrityConfig();
    logger->info("[Preflight] CI config: enabled: {}, debug mode: {}, test signing: {}, HVCI: {}",
                 codeIntegrityConfig.m_codeIntegrity, codeIntegrityConfig.m_debugMode,
                 codeIntegrityConfig.m_testSigning, codeIntegrityConfig.m_hypervisorCodeIntegrity);

    logger->info("[Preflight] Raw CI: {0:#b}", codeIntegrityConfig.m_raw);

    if (!codeIntegrityConfig.m_codeIntegrity || codeIntegrityConfig.m_debugMode || codeIntegrityConfig.m_testSigning ||
        !codeIntegrityConfig.m_hypervisorCodeIntegrity)
    {
        logger->error("[Preflight] System CI violation!");
    }

    for (const auto& driver : shared::EnvironmentIntegrity::GetLoadedDriverPaths())
    {
        if (driver.m_fullPath.empty())
        {
            logger->warn("[Preflight] Note: Driver {} full path is empty and thus not integrity checkable",
                         shared::Util::ToUTF8(driver.m_rawPath));
            continue;
        }

        if (!shared::EnvironmentIntegrity::VerifyFileTrust(driver.m_fullPath,
                                                           shared::EnvironmentIntegrity::EFileType::Driver))
        {
            logger->error("[Preflight] Driver {} has a bad file signature!", shared::Util::ToUTF8(driver.m_rawPath));
        }
    }

    m_hasRunPreflight = true;
}

void pico::Engine::Engine::SetupModuleData() noexcept
{
    // Get module base
    RtlPcToFileHeader(pico::Engine::Engine::Get, &m_moduleBase);

    auto& logger = Logger::GetLogSink();

    auto peHeader = shared::PE::GetImagePtr(m_moduleBase);

    if (!peHeader)
    {
        logger->error("[Engine] Failed to set up module data, module base was not valid PE!");
        return;
    }

    m_moduleSize = peHeader->get_nt_headers()->optional_header.size_image;

    // Get page size, min and max UM addresses and allocation granularity
    Windows::SYSTEM_BASIC_INFORMATION basicInfo{};
    pico::Uint32 sizeWritten{};

    if (!NT_SUCCESS(Windows::NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS::SystemBasicInformation,
                                                      &basicInfo, sizeof(basicInfo), sizeWritten)))
    {
        logger->error("[Engine] NtQuerySystemInformation(SystemBasicInformation, ...) failed");
        return;
    }

    m_pageSize = basicInfo.PageSize;
    m_minimumUMAddress = basicInfo.MinimumUserModeAddress;
    m_maximumUMAddress = basicInfo.MaximumUserModeAddress;
}

pico::Engine::Engine& pico::Engine::Engine::Get() noexcept
{
    static Engine s_instance{};

    return s_instance;
}

pico::Engine::EngineThreadLoadGuard::EngineThreadLoadGuard() noexcept
{
    Engine::Get().m_threadsUnderHeavyLoad++;
}

pico::Engine::EngineThreadLoadGuard::~EngineThreadLoadGuard() noexcept
{
    Engine::Get().m_threadsUnderHeavyLoad--;
}