/**
 * \brief Functions to provide assistance with integration.
 *
 * ====================================================================================================
 */

#pragma once
#include <Shared/Pico.hpp>

namespace pico::Integration
{
/**
 * \brief Returns whether or not we are running inside of a given process name's address space.
 * \param aProcNameHash The hash of the name of the process we are checking for.
 * \returns True if we are loaded inside the process name specified by aProcNameHash, false if not.
 */
pico::Bool IsProcess(pico::shared::HashedString aProcNameHash);

/**
 * \brief Returns whether or not we are running inside CS2's address space.
 * \returns True if we are loaded inside CS2, false if not.
 */
pico::Bool IsCS2();

/**
 * \brief Returns whether or not we are running inside Rust's address space.
 * \returns True if we are loaded inside Rust, false if not.
 */
pico::Bool IsRust();

/**
 * \brief Sleeps until the module is loaded.
 * \param aModuleName The hash of the module name to wait for.
 */
void WaitUntilModuleLoaded(pico::shared::HashedString aModuleName);

// In case of several alternate module name selections (for instance, CS2 has both Vulkan and DX11 as render system
// choices, if one is selected the other won't be loaded) Not terribly well optimized, but it doesn't really matter for
// something that'll run once and won't be used in production

/**
 * \brief Sleeps until at least one of the modules in the specified list is loaded.
 * \param aModuleNames Hashes of the module names to wait for.
 */
void WaitUntilOneOfModulesLoaded(const pico::Vector<pico::shared::HashedString>& aModuleNames);

/**
 * \brief Breaks into the debugger and kills the process afterwards.
 * \param aErrorCode The return value the process will return upon termination.
 */
void Die(pico::Uint32 aErrorCode);
} // namespace pico::Integration