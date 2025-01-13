#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{
/**
 * \brief A scanner of the current process's working set in an attempt to catch manually mapped modules and shellcode.
 *
 * Utilizes pico::shared::MemoryEnv::GetProcessWorkingSet to obtain a snapshot of the process's current working set.
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
    // The maximum size of a non-image executable allocation before we log it.
    static constexpr auto MaxExecutableAllocationSize = 0x2000u;

    // A cache of the process working set.
    // Although non-executable entries might update quite often, executable ones will update much rarer.
    pico::Vector<Windows::MEMORY_WORKING_SET_BLOCK> m_workingSetCache{};

    // A set of already iterated virtual page numbers by VirtualQuery
    // pico::Set<pico::Uint64> m_scannedPages{};

    std::chrono::high_resolution_clock::time_point m_nextWorkingSetCacheUpdate{};

    /**
     * \brief Walks the process working set in search of potential shellcode/manually mapped images.
     */
    void WalkWorkingSet() noexcept;

    /**
     * \brief Processes a page in search for anomalous characteristics.
     *
     * This can probably be optimized in more ways than it currently is. Currently we only scan for private pages
     * instead of handling shared ones as well.
     *
     * \param aBlock The working set entry block. Should be executable.
     */
    void ProcessPage(Windows::MEMORY_WORKING_SET_BLOCK aBlock) noexcept;

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
