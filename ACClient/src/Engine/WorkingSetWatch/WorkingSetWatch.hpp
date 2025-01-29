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
    static constexpr pico::Size BufferSize = 2048u;

    // Is this our first call?
    pico::AtomicBool m_initialized{};
    
    // The buffer to be filled. Size of 2048. 
    std::array<Windows::PROCESS_WS_WATCH_INFORMATION_EX, BufferSize> m_workingSetWatchBuffer{};

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
