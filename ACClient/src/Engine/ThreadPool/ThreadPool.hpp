#pragma once
#include <Shared/Pico.hpp>
#include <BS_thread_pool.hpp>

namespace pico::Engine
{
/**
 * \brief Thread pool wrapper for the detection engine, new jobs are queued every time
 * Pico_PollMainLoop()
 * is called and there are no jobs running.
 * Most detection tasks should run through this.
 * 
 * Note that BS::thread_pool will internally use STL types with a different allocator than ours.
 * (shouldn't a problem with operator new override, however)
 * 
 * This should be fixed in a production environment.
 */
struct ThreadPool : shared::Util::NonCopyableOrMovable
{
    // This is a fairly arbitrary value, but I believe that 8 threads should be okay enough
    inline static constexpr auto MaxThreadCount = 8u;

    BS::thread_pool m_pool{}; // Internal thread pool
    pico::Vector<pico::Uint32> m_threadIds{}; // IDs of threads in pool
    // NOTE: In a production environment, it would be advised to check 
    // the integrity of m_threadIds array

    ThreadPool() noexcept;

    /**
     * \brief Dispatch a job to the thread pool.
     * \tparam Fn The type of the dispatched job. Does not need to be specified.
     * \param aFunc The job that needs to be executed.
     * \param aPriority The priority of the dispatched job. Default is normal.
     */
    template<typename Fn>
    void Dispatch(Fn&& aFunc, BS::priority_t aPriority = BS::pr::normal) noexcept
    {
        m_pool.detach_task(std::forward<Fn>(aFunc), aPriority);
    }

    /**
     * \brief Gets if any jobs should be queued to the thread pool.
     * \return Whether or not it is OK to queue jobs to the thread pool right now.
     */
    pico::Bool CanPushJobs() const noexcept;

    /**
     * \brief Get a singleton instance of the thread pool.
     * \return A singleton instance of the thread pool.
     */
    static ThreadPool& Get() noexcept;
};
}
