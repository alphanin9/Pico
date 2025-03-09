#include <DevIntegration/Integration.hpp>
#include <Engine/ThreadPool/ThreadPool.hpp>

pico::Engine::ThreadPool::ThreadPool()
    : m_pool(std::min(ThreadPool::MaxThreadCount, std::thread::hardware_concurrency()))
{
    m_threadIds.reserve(m_pool.get_thread_count());

    // In our case native handle type is just HANDLE
    for (HANDLE threadHandle : m_pool.get_native_handles())
    {
        Windows::THREAD_BASIC_INFORMATION threadInfo{};

        pico::Uint32 sizeWritten{};

        if (!NT_SUCCESS(Windows::NtQueryInformationThread(threadHandle,
                                                          Windows::THREADINFOCLASS::ThreadBasicInformation, &threadInfo,
                                                          sizeof(threadInfo), sizeWritten)))
        {
            // What even happened?

            Integration::Die(0x51C01010);
        }

        m_threadIds.push_back((pico::Uint32)(threadInfo.ClientId.UniqueThread));
    }

    m_canSubmitJobs = true;
}

pico::Bool pico::Engine::ThreadPool::CanPushJobs() const
{
    // To avoid troublesome situations, we only push more jobs
    // once the pool has handled everything else that we've thrown at it.
    return m_pool.get_tasks_total() == 0u;
}

void pico::Engine::ThreadPool::Teardown()
{
    m_pool.purge();
    m_pool.wait();

    m_canSubmitJobs = false;
}

pico::Engine::ThreadPool& pico::Engine::ThreadPool::Get()
{
    static ThreadPool s_instance{};

    return s_instance;
}