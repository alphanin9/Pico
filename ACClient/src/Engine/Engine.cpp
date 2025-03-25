#include <Engine/ContextScanner/ContextScanner.hpp>
#include <Engine/DriverSnap/DriverSnap.hpp>
#include <Engine/Engine.hpp>
#include <Engine/HandleSnap/HandleSnap.hpp>
#include <Engine/InstrumentationCallbacks/InstrumentationCallbacks.hpp>
#include <Engine/IntegrityChecker/IntegrityChecker.hpp>
#include <Engine/Logging/Logger.hpp>
#include <Engine/ProcessSnap/ProcessSnap.hpp>
#include <Engine/ThreadPool/ThreadPool.hpp>
#include <Engine/WindowSnap/WindowSnap.hpp>
#include <Engine/WorkingSetScanner/WorkingSetScanner.hpp>
#include <Engine/WorkingSetWatch/WorkingSetWatch.hpp>

#include <DevIntegration/Integration.hpp>

#include <Engine/Specific/CS2/CS2.hpp>
void pico::Engine::Engine::Tick()
{
    if (!m_canExecute)
    {
        return;
    }

    TickMainThreadJobs();

    auto& threadPool = ThreadPool::Get();

    if (!threadPool.CanPushJobs())
    {
        // Don't add new jobs yet...
        return;
    }

    // Dispatch jobs here.
    // Note: consider doing something like a counter for job dispatching, add two jobs to threadpool every time counter
    // spins.
    // Keep logger job always running, though, that one's important - maybe WS watch too, it needs fresh info.

    threadPool.Dispatch([]() { WorkingSetWatcher::Get().Tick(); });
    threadPool.Dispatch([]() { Logger::Get().Tick(); });

    switch (m_jobState)
    {
    case 0:
    {
        threadPool.Dispatch([]() { ContextScanner::Get().Tick(); });
        threadPool.Dispatch([]() { InstrumentationCallbacks::Get().Tick(); });
        break;
    }
    case 1:
    {
        threadPool.Dispatch([]() { IntegrityChecker::Get().Tick(); });
        break;
    }
    case 2:
    {
        threadPool.Dispatch([]() { WorkingSetScanner::Get().Tick(); });
        threadPool.Dispatch([]() { DriverSnap::Get().Tick(); });
        break;
    }
    case 3:
    {
        threadPool.Dispatch([]() { HandleSnap::Get().Tick(); });
        threadPool.Dispatch([]() { ProcessSnap::Get().Tick(); });
        break;
    }
    case 4:
    {
        static const auto s_isCS2 = Integration::IsCS2();

        if (s_isCS2)
        {
            threadPool.Dispatch([]() { Specific::CS2::Get().Tick(); });
        }

        threadPool.Dispatch([]() { WindowSnap::Get().Tick(); });
        break;
    }
    default:
        break;
    }

    m_jobState = (m_jobState + 1) % MaxJobStates;
}

void pico::Engine::Engine::TickMainThreadJobs()
{
    // Modulos for timestamp counter to check things
    constexpr auto ThreadPoolCheck = 0x51Cu;
    constexpr auto ThreadCtxCheck = 0xC15u;

    const auto timestampCounter = __rdtsc();

    auto& logger = Logger::GetLogSink();

    if ((timestampCounter % ThreadPoolCheck) == 0u && !IsThreadPoolOK())
    {
        // In production we should report the issue and crash
        logger->error("[Engine] Thread pool had issues!");
    }

    if ((timestampCounter % ThreadCtxCheck) == 0u)
    {
        ContextScanner::Get().TickMainThread();
    }

    InstrumentationCallbacks::Get().TickMainThread();
}

pico::Bool pico::Engine::Engine::IsThreadPoolOK()
{
    static auto& s_threadPool = ThreadPool::Get();
    auto& logger = Logger::GetLogSink();

    // Dispatch job to all threads
    // NOTE: on second thoughts this is a bit silly,
    // but it should still catch suspended threads every now and then

    if (m_threadsUnderHeavyLoad < 0)
    {
        logger->critical("[Engine] Engine::m_threadsUnderHeavyLoad is negative, this should NEVER happen!");
    }

    constexpr auto CheckForBrokenPoolConstant = 0x12C01;

    // While this is not perfect at all, this check should help out with stutters a bit
    if (m_threadsUnderHeavyLoad <= 0 && (__rdtsc() % CheckForBrokenPoolConstant) == 0u)
    {
        const auto jobCount = s_threadPool.m_pool.get_tasks_running();

        std::atomic<pico::Int64> counter{};
        for (auto i = 0u; i < s_threadPool.m_pool.get_thread_count(); i++)
        {
            s_threadPool.Dispatch([&counter]() { counter++; }, BS::pr::highest);
        }

        // Given this is called before threadPool.CanPushJobs(),
        // this may also end up waiting on normal worker threads - good
        // Not actually very good, given we can have long-running jobs here!

        const auto start = Clock::now();

        // We should not timeout forever ever
        // However, 1s of timeout time being reported kinda tells you everything you need to know
        constexpr auto MaxTimeOutDuration = 1000u;

        s_threadPool.m_pool.wait_for(pico::Milliseconds{MaxTimeOutDuration});

        const auto duration = std::chrono::duration_cast<pico::Milliseconds>(Clock::now() - start).count();

        logger->info("[Engine] Time taken to wait: {}ms, jobs: {}", duration, jobCount);

        if (duration > 20)
        {
            logger->error("[Engine] Unacceptable stutter from pool wait! This is a PROBLEM.");
        }

        // This should never happen
        if (s_threadPool.m_pool.get_thread_count() != counter)
        {
            logger->error("[Engine] Not all threads responded to pool");
            return false;
        }
    }

    for (HANDLE i : s_threadPool.m_pool.get_native_handles())
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

void pico::Engine::Engine::Setup()
{
    if (m_canExecute)
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
        logger->error("[Preflight] Secure Boot is NOT enabled! This should not let the app launch.");
    }

    logger->info("[Preflight] Dumping Measured Boot config...");

    const auto measuredBootData = shared::EnvironmentIntegrity::GetLastMeasuredBootData();

    Logger::Get().DumpDataToFile(L"MeasuredBootDataJSON", (void*)(measuredBootData.m_json.data()),
                                 measuredBootData.m_json.size());
    Logger::Get().DumpDataToFile(L"MeasuredBootDataRaw", (void*)(measuredBootData.m_rawBuffer.data()),
                                 measuredBootData.m_rawBuffer.size());

    const auto codeIntegrityConfig = shared::EnvironmentIntegrity::GetCodeIntegrityConfig();
    logger->info("[Preflight] CI config: enabled: {}, debug mode: {}, test signing: {}, HVCI: {}",
                 codeIntegrityConfig.m_codeIntegrity, codeIntegrityConfig.m_debugMode,
                 codeIntegrityConfig.m_testSigning, codeIntegrityConfig.m_hypervisorCodeIntegrity);

    logger->info("[Preflight] Raw CI: {0:#b}", codeIntegrityConfig.m_raw);

    if (!codeIntegrityConfig.m_codeIntegrity || codeIntegrityConfig.m_debugMode || codeIntegrityConfig.m_testSigning ||
        !codeIntegrityConfig.m_hypervisorCodeIntegrity)
    {
        logger->error("[Preflight] System CI violation! This should not let the app launch.");
    }

    if (!DriverSnap::Get().OnPreflight())
    {
        logger->error("[Preflight] Driver violation detected! This should not let the app launch.");
    }

    m_canExecute = true;
}

void pico::Engine::Engine::SetupModuleData()
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

void pico::Engine::Engine::Teardown()
{
    m_canExecute = false;

    auto& pool = ThreadPool::Get();
    auto& instrumentationCallbacks = InstrumentationCallbacks::Get();

    pool.Teardown();
    instrumentationCallbacks.Teardown();
}

pico::Engine::Engine& pico::Engine::Engine::Get()
{
    static Engine s_instance{};

    return s_instance;
}

pico::Engine::EngineThreadLoadGuard::EngineThreadLoadGuard()
{
    Engine::Get().m_threadsUnderHeavyLoad++;
}

pico::Engine::EngineThreadLoadGuard::~EngineThreadLoadGuard()
{
    Engine::Get().m_threadsUnderHeavyLoad--;
}