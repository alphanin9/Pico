#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>

namespace pico
{
namespace shared
{
namespace EnvironmentIntegrity
{
/**
 * \brief The current Secure Boot config as queried by NtQuerySystemInformation.
 *
 * In production this should be validated by other sources.
 */
struct SecureBootConfig
{
    // Is Secure Boot enabled?
    pico::Bool m_secureBootEnabled{};

    // Is Secure Boot supported at all?
    pico::Bool m_secureBootSupported{};
};

/**
 * \brief Queries the Secure Boot config and returns it.
 * \return The Secure Boot config.
 */
SecureBootConfig GetSecureBootConfig();

/**
 * \brief Data from Windows's Measured Boot logs in %SystemRoot%/Logs/MeasuredBoot.
 *
 * This can shed more light on Secure Boot state.
 */
struct MeasuredBootData
{
    pico::String m_json{};
    pico::Vector<pico::Uint8> m_rawBuffer{};
};

/**
 * \brief Finds the latest Measured Boot data and dumps it to a vector of bytes.
 *
 * This should, along with the Secure Boot configuration, be validated with several sources.
 *
 * \return The latest raw Measured Boot data.
 */
MeasuredBootData GetLastMeasuredBootData();
} // namespace EnvironmentIntegrity
} // namespace shared

} // namespace pico
