#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{
/**
 * \brief A module to detect potential external hacks. Walks through the system handle information and attempts to
 * enumerate the handles open to us with dangerous rights (reading virtual memory/writing virtual memory/creating a new
 * thread in us).
 *
 * As we do not operate on higher privilege levels (theoretically, we could have a service enumerate handles for us or
 * make the user start the process as administrator and then we would have capability to get info for every process -
 * but that's more complication than is needed for a thesis), we will not be able to catch all processes. This should be
 * rectified in production or mitigated through the use of a kernel driver to ensure system integrity.
 */
struct HandleSnap : public shared::Util::NonCopyableOrMovable
{
    pico::Timestamp m_lastCheckTime{};

    /**
     * \brief Tick component in thread pool.
     */
    void Tick() noexcept;

    /**
     * \brief Get a singleton instance of the integrity checker.
     * \return A singleton instance of the integrity checker.
     */
    static HandleSnap& Get() noexcept;
};
} // namespace pico::Engine