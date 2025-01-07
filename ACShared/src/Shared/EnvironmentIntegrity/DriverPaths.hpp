#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>
namespace pico::shared::EnvironmentIntegrity
{
/**
 * \brief Minor information about a loaded kernel mode driver.
 */
struct DriverInfo
{
    // Raw path to driver, as specified in module information struct
    pico::UnicodeString m_rawPath{};

    // Full path to driver
    pico::UnicodeString m_fullPath{};
};

/**
 * \brief Gets currently loaded kernel mode drivers.
 * 
 * Drivers are obtained via the NtQuerySystemInformation(SYSTEM_MODULE_INFORMATION, ...) system call.
 * 
 * In production, this should probably also be queried and validated via the kernel component.
 * 
 * \return A vector of structs describing currently loaded drivers.
 * If the function fails, it returns an empty vector.
 */
pico::Vector<DriverInfo> GetLoadedDriverPaths() noexcept;
}