#include <Engine/ContextScanner/ContextScanner.hpp>
#include <Engine/Engine.hpp>
#include <Engine/Logging/Logger.hpp>
#include <Engine/ThreadPool/ThreadPool.hpp>

void pico::Engine::ContextScanner::TickMainThread() noexcept
{
    const auto& engine = Engine::Get();
    const auto& threadPool = ThreadPool::Get();

    auto& logger = Logger::Get();

    // Vector might actually be faster here due to cache
    pico::Set<pico::Uint32> whitelistedIds{threadPool.m_threadIds.begin(), threadPool.m_threadIds.end()};

    static const auto s_pid = shared::ProcessEnv::GetCurrentThreadEnvironment()->ClientId.UniqueProcess;

    // Not sure if we can make this static
    static const auto currentThreadId = shared::ProcessEnv::GetCurrentThreadEnvironment()->ClientId.UniqueThread;

    pico::Vector<pico::Uint32> selection{};

    shared::SystemEnv::EnumerateRunningProcesses(
        [this, &selection, &whitelistedIds](Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION* aProc,
                                            const pico::Vector<Windows::SYSTEM_EXTENDED_THREAD_INFORMATION*>& aThreads)
        {
            if (reinterpret_cast<pico::Uint64>(aProc->UniqueProcessId) != s_pid)
            {
                return;
            }

            for (auto thread : aThreads)
            {
                auto threadId = reinterpret_cast<pico::Uint64>(thread->ThreadInfo.ClientId.UniqueThread);

                if (threadId == currentThreadId)
                {
                    continue;
                }

                if (whitelistedIds.contains(static_cast<pico::Uint32>(threadId)))
                {
                    continue;
                }

                selection.push_back(static_cast<pico::Uint32>(threadId));
            }
        });

    if (selection.empty())
    {
        logger.m_logger->info("No threads found!");
        return;
    }

    static std::random_device s_random{};
    static std::mt19937 s_mt{s_random()};

    std::uniform_int_distribution<pico::Size> dist{0u, selection.size() - 1u};

    auto threadId = selection[dist(s_mt)];

    wil::unique_handle threadHandle{
        OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION | THREAD_SUSPEND_RESUME, false, threadId)};

    if (!threadHandle.is_valid())
    {
        logger.m_logger->info("Failed to open handle to thread {}!", threadId);
        return;
    }

    if (SuspendThread(threadHandle.get()) == std::numeric_limits<pico::Uint32>::max())
    {
        logger.m_logger->error("Failed to suspend thread {}!", threadId);
        return;
    }

    CONTEXT ctx{};

    // Only dump some registers
    ctx.ContextFlags = CONTEXT_CONTROL;

    if (!GetThreadContext(threadHandle.get(), &ctx))
    {
        logger.m_logger->error("Failed to get thread {} context!", threadId);
        return;
    }

    // We don't want to keep the thread suspended for too long, so we don't set lock while copying ctx

    auto frame = pico::MakeShared<ContextFrame>();

    frame->m_threadId = threadId;
    frame->m_rip = ctx.Rip;
    frame->m_rsp = ctx.Rsp;

    // Align the stack pointer down, so we catch the whole page
    // Stack grows downwards
    // Yes, we'll catch some unitialized memory - shouldn't matter
    // And yes, having dynamic allocation on dynamic allocation is a bit silly too
    frame->m_stackPage.assign(engine.m_pageSize / sizeof(void*), {});

    auto pageLow = shared::Util::AlignDown(frame->m_rsp, static_cast<pico::Uint64>(engine.m_pageSize));

    // Copy stack into frame
    std::copy_n(reinterpret_cast<void**>(pageLow), frame->m_stackPage.size(), frame->m_stackPage.begin());

    ResumeThread(threadHandle.get());

    logger.m_logger->info("Dumped thread {} stack!", threadId);

    PushFrame(frame);
}

pico::Bool IsProtectionExecutable(pico::Uint32 aProtection)
{
    return aProtection == PAGE_EXECUTE || aProtection == PAGE_EXECUTE_READ || aProtection == PAGE_EXECUTE_READWRITE ||
           aProtection == PAGE_EXECUTE_WRITECOPY;
}

void pico::Engine::ContextScanner::Tick() noexcept
{
    const auto& engine = Engine::Get();

    auto& logger = Logger::Get();

    pico::Vector<pico::SharedPtr<ContextFrame>> contextFrames{};

    {
        std::unique_lock lock(m_contextScannerMutex);

        contextFrames = std::move(m_frames);
        m_frames = {};
    }

    auto currentProcess = GetCurrentProcess();

    for (auto& frame : contextFrames)
    {
        // Check RIP
        // Walk through stack, search for executable addrs

        for (auto addy : frame->m_stackPage)
        {
            // No point worrying about weird KM stuff like perfect injector ATM

            if (reinterpret_cast<uintptr_t>(addy) > engine.m_maximumUMAddress ||
                reinterpret_cast<uintptr_t>(addy) < engine.m_minimumUMAddress)
            {
                continue;
            }

            MEMORY_BASIC_INFORMATION info{};

            // Should also check if page is readable via SEH and pointer deref
            // If VirtualQuery fails but that doesn't, something is definitely odd and it deserves attention
            // (and dumping)
            if (!VirtualQuery(addy, &info, sizeof(info)))
            {
                // It's probably not an address to anywhere relevant
                continue;
            }

            // It's a return address? Is someone trying to screw with protection?
            if (IsProtectionExecutable(info.Protect) || IsProtectionExecutable(info.AllocationProtect))
            {
                shared::PE::Image* peImage{};

                // Is this a proper PE file? (note: this is a hack, we should be checking info.Type)
                RtlPcToFileHeader(addy, reinterpret_cast<PVOID*>(&peImage));

                if (!peImage)
                {
                    logger.m_logger->error(
                        "Executable page of memory ({}, base {}) does not have associated PE file, is this JIT or manual map/shellcode?", addy, info.AllocationBase);
                    continue;
                }

                pico::UnicodeString moduleName{};

                if (FAILED(wil::GetModuleFileNameW(reinterpret_cast<HMODULE>(peImage), moduleName)))
                {
                    logger.m_logger->error("Failed to get name of module {}!", reinterpret_cast<void*>(peImage));
                    continue;
                }

                logger.m_logger->info("Found address {} from {} (base {}) on stack of thread!", addy,
                                      shared::Util::ToUTF8(moduleName), reinterpret_cast<void*>(peImage));
            }
        }
    }
}

void pico::Engine::ContextScanner::PushFrame(pico::SharedPtr<ContextFrame>& aFrame) noexcept
{
    std::unique_lock lock(m_contextScannerMutex);

    m_frames.push_back(aFrame);
}

pico::Engine::ContextScanner& pico::Engine::ContextScanner::Get() noexcept
{
    static ContextScanner s_instance{};

    return s_instance;
}