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
 * If the page does not belong to a section and its allocation size is larger than 16kB, we log the offending memory region.
 * In production, we should also send the offending pages to the backend for further analysis.
 * 
 * Note that this may fail if the image has a kernel mode driver attempting to hide it with VAD manipulation/whatnot.
 */
struct WorkingSetScanner : shared::Util::NonCopyableOrMovable
{
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
}
