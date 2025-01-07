#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>

namespace pico::shared::MemoryEnv
{
/**
 * \brief Queries the current process's working set.
 * \return A vector of entries detailing the current process's working set. If the function fails, it returns an empty vector.
 */
pico::Vector<Windows::MEMORY_WORKING_SET_BLOCK> GetProcessWorkingSet() noexcept;

/**
 * \brief Checks if the memory protection as queried by VirtualQuery is executable.
 * \param aProtection The memory protection value.
 * \return Whether or not the memory protection is executable.
 */
pico::Bool IsProtectionExecutable(pico::Uint32 aProtection) noexcept;

}