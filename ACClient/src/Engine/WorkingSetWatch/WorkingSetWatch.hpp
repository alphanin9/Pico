#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{
/**
 * \brief Utilizes Windows's working set watch capabilities to look for page faults caused by invalid or process-external thread IDs.
 * 
 * Note: currently I don't know if this works on page faults caused by other processes - according to static analysis of KiPageFault, it should.
 */
struct WorkingSetWatcher : public shared::Util::NonCopyableOrMovable
{
    // Is this our first call?
    pico::AtomicBool m_initialized{};

    // Lock this when accessing m_workingSetBuffer.
    std::mutex m_workingSetWatchBufferLock{};

    // The buffer to be filled.
    pico::Vector<Windows::PROCESS_WS_WATCH_INFORMATION_EX> m_workingSetWatchBuffer{};

    /**
     * \brief Roll the process into working set watch and obtain new working set records. This might need to be moved off the main thread eventually.
     */
    void TickMainThread() noexcept;

    /**
     * \brief Tick component in thread pool worker.
     */
    void Tick() noexcept;

    /**
     * \brief Get a singleton instance of the working set watcher.
     * 
     * \return A singleton instance of the working set watcher.
     */
    static WorkingSetWatcher& Get() noexcept;
};
}
