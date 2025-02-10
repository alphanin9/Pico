#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>

namespace pico::shared::EnvironmentIntegrity
{
/**
 * \brief Defines which subsystem will be used for file verification.
 */
enum class EFileType
{
    Other = 0, // A regular executable/whatever digitally signed file.
    Driver = 1 // A kernel mode driver.
};

/**
 * \brief Verifies a file's digital signature using the WinVerifyTrust method. This is relatively slow.
 *
 * \param aPath The path to the file to be verified.
 * \param aType The system used to perform the file verification check. Other by default.
 *
 * \return Whether the file's digital signature is valid or not.
 */
pico::Bool VerifyFileTrust(pico::UnicodeStringView aPath, EFileType aType = EFileType::Other);

/**
 * \brief Verifies a file's digital signature based on a catalog file. This is relatively slow.
 *
 * \param aPath The path to the file to be verified.
 * \param aType The system used to perform the file verification check. Other by default.
 *
 * \return Whether the file's digital signature as listed in the catalog is valid or not.
 */
pico::Bool VerifyFileTrustFromCatalog(pico::UnicodeStringView aPath, EFileType aType = EFileType::Other);
} // namespace pico::shared::EnvironmentIntegrity