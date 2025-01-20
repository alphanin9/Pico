#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>
#include <Shared/Util/NtdllCalls.hpp>

namespace pico::shared::SystemEnv
{
/**
 * \brief Gets all currently opened handles in the system. This may be slow.
 * \return A vector of every open handle at the time of calling. If the function failed for any reason, returns an empty
 * vector.
 */
pico::Vector<Windows::SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX> GetHandleTable() noexcept;
} // namespace pico::shared::SystemEnv
