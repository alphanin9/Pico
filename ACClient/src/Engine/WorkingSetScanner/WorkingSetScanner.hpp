#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{
/**
 * \brief A scanner of the current process's working set in an attempt to catch manually mapped modules and shellcode.
 *
 * Utilizes the shared library to obtain our current process's working set.
 *
 * We then proceed to walk it to find executable pages. If a page is executable, we call VirtualQuery on it to obtain
 * whether or not the page belongs to a section, along with its allocation size.
 *
 * If the page does not belong to a section and its allocation size is larger than 2kB, we log the offending memory
 * region. In production, we should also send the offending pages to the backend for further analysis. This may cause
 * false positives with detour trampolines.
 *
 * Note that this may fail if the image has a kernel mode driver attempting to hide it with VAD manipulation/whatnot.
 */
struct WorkingSetScanner : public shared::Util::NonCopyableOrMovable
{
    // How often the memory buffer needs to be refreshed.
    static constexpr pico::Seconds MemoryCacheRefreshTime{30};

    // How often the process working set should be updated.
    static constexpr pico::Seconds WorkingSetUpdateTime{5};

    // The maximum size of a non-image executable allocation before we log it.
    static constexpr pico::Size MaxExecutableAllocationSize = 0x2000u;

    // Whether or not the component is currently running its tick function. (TODO, USELESS NOW)
    pico::AtomicBool m_isExecuting{};

    // The raw buffer containing the process working set entries. (TODO)
    std::array<pico::Uint64, 16777216u> m_workingSetBuffer{};

    // A pointer to the start of the working set buffer.
    Windows::MEMORY_WORKING_SET_INFORMATION* m_workingSetInfo{};

    // A cache of the process working set.
    // Although non-executable entries might update quite often, executable ones will update much rarer.
    pico::Vector<Windows::MEMORY_WORKING_SET_BLOCK> m_workingSetCache{};

    // The index from the previous iteration
    pico::Size m_lastIndex{};

    // Whether or not we're already done
    // Initialized to true to avoid bad scans if we fail on start
    pico::Bool m_doneWithScan = true;

    pico::Timestamp m_nextWorkingSetCacheUpdate{};

    /**
     * \brief Walks the process working set in search of potential shellcode/manually mapped images.
     */
    void WalkWorkingSet() noexcept;

    /**
     * \brief Updates the process working set.
     * \return Whether or not querying an update of the working set succeeded.
     */
    pico::Bool UpdateWorkingSet() noexcept;

    /**
     * \brief Ticks component in the thread pool worker.
     */
    void Tick() noexcept;

    /**
     * \brief Gets an instance of the working set scanner.
     *
     * \return A singleton instance of the working set scanner.
     */
    static WorkingSetScanner& Get() noexcept;
};
} // namespace pico::Engine
