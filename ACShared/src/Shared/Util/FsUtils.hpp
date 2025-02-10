#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>

namespace pico::shared::Files
{
/**
 * \brief Expands a driver's file path (in the format of \\\SystemRoot\\System32\\Driver.sys) to its full state.
 * \param aPath The path to be expanded.
 * \return A full path to the driver's file. If the function fails, it returns an empty string.
 */
pico::UnicodeString GetFullDriverFilePath(pico::UnicodeStringView aPath);

/**
 * \brief Calculate a file's SHA256 hash.
 * \param aPath The path to the file the hash of which needs to be calculated.
 * \return A hex string containing the hash of the file. If the function fails, it returns an empty string.
 */
pico::String GetFileSHA256(pico::UnicodeStringView aPath);
} // namespace pico::shared::Files
