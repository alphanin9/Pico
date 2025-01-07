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
 * \brief Code integrity settings from NtQuerySystemInformation
 * 
 * Note: In a production environment, these should also be double-checked by the kernel-mode component
 * 
 * Other UM-available sources should be checked as well
 */
struct CodeIntegrityConfig
{
    // Is code integrity enabled at all?
    pico::Bool m_codeIntegrity{};

    // Is hypervisor-enforced code integrity enabled?
    pico::Bool m_hypervisorCodeIntegrity{};

    // Is test signing enabled?
    pico::Bool m_testSigning{};

    // Is debug mode enabled?
    pico::Bool m_debugMode{};

    // Raw value
    pico::Uint32 m_raw{};
};

CodeIntegrityConfig GetCodeIntegrityConfig() noexcept;
} // namespace EnvironmentIntegrity
} // namespace shared
} // namespace pico
