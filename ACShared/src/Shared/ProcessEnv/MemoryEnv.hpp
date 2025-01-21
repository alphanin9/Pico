#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>

namespace pico::shared::MemoryEnv
{
/**
 * \brief Queries the current process's working set.
 * \return A vector of entries detailing the current process's working set. If the function fails, it returns an empty
 * vector.
 */
pico::Vector<Windows::MEMORY_WORKING_SET_BLOCK> GetProcessWorkingSet() noexcept;

/**
 * \brief Queries the current process's working set.
 *
 * Instead of outputting a vector of process working set entries,
 * fills a buffer pointed to by aBuffer with Windows working set information directly. In theory, this should cause
 * fewer allocations.
 *
 * \param aBuffer The buffer working set information will be placed into.
 *
 * \return Whether or not filling the buffer succeeded.
 */
pico::Bool FillProcessWorkingSetBuffer(pico::Vector<pico::Uint8>& aBuffer) noexcept;

/**
 * \brief Checks if the memory protection as queried by VirtualQuery is executable.
 * \param aProtection The memory protection value.
 * \return Whether or not the memory protection is executable.
 */
pico::Bool IsProtectionExecutable(pico::Uint32 aProtection) noexcept;

} // namespace pico::shared::MemoryEnv