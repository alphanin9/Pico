#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{

struct ThreadInformation
{

};

struct ProcessInformation
{
    
};

/**
 * \brief Process enumeration to catch potential blacklisted applications or gather details on external hacks that have
 * handles opened to us.
 *
 * Dumps some information about (mostly) every process open on the system (PID, full path, running threads...)
 * In the future, can be extended to dump more information about a process (PE structure, debug symbol path...)
 */
struct ProcessSnap : public shared::Util::NonCopyableOrMovable
{
    // How often the process cache should be refreshed.
    static constexpr pico::Seconds ProcessCacheRefreshTime{5};

    // Whether or not the component is currently running its tick function. (TODO)
    pico::AtomicBool m_isExecuting{};

    /**
     * \brief Tick component in thread pool worker.
     */
    void OnTick() noexcept;
};
}
