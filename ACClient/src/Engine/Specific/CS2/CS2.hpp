#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine::Specific
{
/**
 * \brief Counter-Strike 2 specific methods. Should only be used for Counter-Strike 2.
 *
 * Includes module interface list walking to find shadow VMT hooks.
 */
struct CS2 : public shared::Util::NonCopyableOrMovable
{
    /**
     * \brief Walks through the interface list of modules exporting the CreateInterface function.
     *
     * The interfaces' vtables will be checked for integrity. Utilizes tier0.dll!Plat_GetRegisteredModules to
     * get a list of important modules.
     */
    void WalkInterfaces();

    /**
     * \brief Tick component in thread pool.
     */
    void Tick();

    /**
     * \brief Get a singleton instance of the CS2-specific detections.
     * \return A singleton instance of the CS2-specific detections.
     */
    static CS2& Get();
};
} // namespace pico::Engine::Specific