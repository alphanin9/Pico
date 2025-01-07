#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>

namespace pico::shared::Files
{
/**
 * \brief Expands a driver's file path (in the format of \\\SystemRoot\\System32\\Driver.sys) to its full state.
 * \param aPath The path to be expanded.
 * \return A full path to the driver's file.
 */
pico::UnicodeString GetFullDriverFilePath(pico::UnicodeStringView aPath) noexcept;
}
