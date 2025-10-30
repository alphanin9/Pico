#pragma once
#include <BS_thread_pool.hpp>
#include <Shared/Pico.hpp>

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
struct ThreadPool : public shared::Util::NonCopyableOrMovable
{
    // This is a fairly arbitrary value, but I believe that 8 threads should be okay enough
    inline static constexpr auto MaxThreadCount = 8u;

    BS::thread_pool<BS::tp::priority> m_pool{}; // Internal thread pool
    pico::Vector<pico::Uint32> m_threadIds{};   // IDs of threads in pool
    // NOTE: In a production environment, it would be advised to check
    // the integrity of m_threadIds array

    // Whether or not jobs can be submitted to the pool.
    pico::Bool m_canSubmitJobs{};

    ThreadPool();

    /**
     * \brief Dispatch a job to the thread pool.
     * \tparam Fn The type of the dispatched job. Does not need to be specified.
     * \param aFunc The job that needs to be executed.
     * \param aPriority The priority of the dispatched job. Default is normal.
     */
    template<typename Fn>
    void Dispatch(Fn&& aFunc, BS::priority_t aPriority = BS::pr::normal)
    {
        if (m_canSubmitJobs)
        {
            m_pool.detach_task(std::forward<Fn>(aFunc), aPriority);
        }
    }

    /**
     * \brief Gets if any jobs should be queued to the thread pool.
     * \return Whether or not it is OK to queue jobs to the thread pool right now.
     */
    pico::Bool CanPushJobs() const;

    /**
     * \brief Shuts the thread pool down. Jobs can no longer be submitted afterwards.
     */
    void Teardown();

    /**
     * \brief Get a singleton instance of the thread pool.
     * \return A singleton instance of the thread pool.
     */
    static ThreadPool& Get();
};
} // namespace pico::Engine
