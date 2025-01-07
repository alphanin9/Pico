/*
 * Functions for the protected application to call for initialization/polling of main loop,
 * in real environment these should preferably be hidden somewhere in the game's online service SDK
 * Like in Easy Anti-Cheat EOS SDK's case
 * However, we do not have an online service SDK and are *not* Easy Anti-Cheat, so we have to be dumb
 *
 * ===================================================================================================
 */

#include <DevIntegration/Integration.hpp>
#include <Engine/Engine.hpp>
#include <Shared/Pico.hpp>

/* Call anti-cheat bootstrapping
 * Should only be called once, but we still make sure it is only called once
 *
 * ===================================================================================================
 */

extern "C" _declspec(dllexport) void Pico_Init()
{
    // Only call this once
    static std::once_flag s_initialized{};

    std::call_once(s_initialized,
                   []()
                   {
                       pico::Engine::Engine::Get().Setup();
                   });
}

/* Call this in every frame of the game's main loop
 * NOTE: should not contain any heavy code
 * Schedule tasks on thread pool threads? (logging, various scans...)
 * Check thread pool thread states
 * Re-call various important functions
 *
 * ===================================================================================================
 */
extern "C" _declspec(dllexport) void Pico_PollMainLoop()
{
    pico::Engine::Engine::Get().Tick();
}