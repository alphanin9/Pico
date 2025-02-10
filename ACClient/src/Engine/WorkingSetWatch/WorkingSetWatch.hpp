#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{
/**
 * \brief Utilizes Windows's working set watch capabilities to look for page faults caused by invalid instruction
 * pointers.
 *
 * Can be improved by a kernel driver directly accessing page fault history without filtering out kernel mode addresses.
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
    void Tick();

    /**
     * \brief Get a singleton instance of the working set watcher.
     *
     * \return A singleton instance of the working set watcher.
     */
    static WorkingSetWatcher& Get();
};
} // namespace pico::Engine
