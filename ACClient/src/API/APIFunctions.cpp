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


extern "C" _declspec(dllexport) void Pico_PollMainLoop()
{
    pico::Engine::Engine::Get().Tick();
}

extern "C" _declspec(dllexport) void Pico_Teardown()
{
    pico::Engine::Engine::Get().Teardown();
}
