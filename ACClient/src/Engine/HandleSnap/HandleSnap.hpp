#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{
/**
 * \brief A module to detect potential external hacks. Walks through the system handle information and attempts to
 * enumerate the handles open to our process or threads with dangerous rights (reading virtual memory/writing virtual
 * memory/creating a new thread in us).
 *
 * As we do not operate on higher privilege levels (theoretically, we could have a service enumerate handles for us or
 * make the user start the process as administrator and then we would have capability to get info for every process -
 * but that's more complication than is needed for a thesis), we will not be able to catch all processes. This should be
 * rectified in production or mitigated through the use of a kernel driver to ensure system integrity.
 */
struct HandleSnap : public shared::Util::NonCopyableOrMovable
{
    pico::Timestamp m_lastCheckTime{};

    // Our last index in the handle table
    pico::Size m_lastHandleIndex{};

    // Are we done with this round?
    pico::Bool m_isDone{};

    // A cache of handle entries
    pico::Vector<Windows::SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX> m_handleEntryCache{};

    /**
     * \brief Tick component in thread pool.
     */
    void Tick() noexcept;

    /**
     * \brief Called when a process handle needs to be checked in the tick function's handle enumeration.
     *
     * \param aEntry The handle table entry.
     * \param aOwner The owner of the handle in question.
     */
    void OnProcessHandleCheck(const Windows::SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX& aEntry,
                              const wil::unique_handle& aOwner) noexcept;

    /**
     * \brief Called when a thread handle needs to be checked in the tick function's handle enumeration.
     *
     * \param aEntry The handle table entry.
     * \param aOwner The owner of the handle in question.
     */
    void OnThreadHandleCheck(const Windows::SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX& aEntry,
                             const wil::unique_handle& aOwner) noexcept;

    /**
     * \brief Get a singleton instance of the integrity checker.
     * \return A singleton instance of the integrity checker.
     */
    static HandleSnap& Get() noexcept;
};
} // namespace pico::Engine