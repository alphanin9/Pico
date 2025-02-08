#include <DevIntegration/Integration.hpp>
#include <DevIntegration/Rust/Rust.hpp>
#include <API/APIFunctions.hpp>

void pico::Integration::Rust::OnReflex(void* aRcx)
{
    static auto s_initialized = false;

    if (!s_initialized)
    {
        Pico_Init();
        s_initialized = true;
    }

    Pico_PollMainLoop();
}

pico::Bool pico::Integration::Rust::InitRust() noexcept
{
    // Note: bad sig, GameAssembly is just generally rough to sig down
    constexpr auto OnReflexFunc =
        pico::shared::RawFunc<"GfxPluginNVIDIAReflex.dll", "\x48\x83\xEC\xCC\xE8\xCC\xCC\xCC\xCC\x48\x85\xC0",
                              pico::Bool (*)(void*)>();

    Hook::HookBefore<OnReflexFunc>(&OnReflex).OrDie();

    return true;
}