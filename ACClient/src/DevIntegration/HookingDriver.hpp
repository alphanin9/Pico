#pragma once
#include <Shared/Pico.hpp>

namespace pico::Integration
{
/**
 * \brief An interface class to provide a driver for hooking.
 */
struct HookingDriver
{
    /// @brief Create a hook with no original function necessary.
    /// @param aAddy The address of the target function.
    /// @param aDetour The address of the function you want to detour execution to.
    /// @return true if hooking succeeded, false if it did not.
    virtual pico::Bool Attach(uintptr_t aAddy, _In_ void* aDetour) = 0;

    /// @brief Create a hook with a specified original function.
    /// @param aAddy The address of the target function.
    /// @param aDetour The address of the function you want to detour execution to.
    /// @param aOriginal A pointer to the trampoline to call the original function.
    /// @return true if hooking succeeded, false if it did not.
    virtual pico::Bool Attach(uintptr_t aAddy, _In_ void* aDetour, _Out_ void** aOriginal) = 0;

    /// @brief Detach from an already created hook.
    /// @param aAddy The address of the target function.
    /// @return true if the hook was able to be destroyed, false if it was not.
    virtual pico::Bool Detach(uintptr_t aAddy) = 0;

    /**
     * \brief Get if this hook was made by us or not.
     *
     * \param aAddy The address we found a hook at.
     * \return The size of the byte patch if it's ours, zero if it's not ours.
     */
    virtual pico::Size GetPatchSize(uintptr_t aAddy) = 0;

    /// @brief Get the default hooking driver (SafetyHookDriver at the moment)
    /// @return An object instantiating the HookingDriver interface
    static HookingDriver* GetDefault();
};
} // namespace pico::Integration
