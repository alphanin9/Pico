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
        pico::shared::RawFunc<"GameAssembly.dll",
                              "\x40\x56\x57\x48\x83\xEC\xCC\x80\x3D\xCC\xCC\xCC\xCC\xCC\x48\x8B\xF9\x75\xCC\x48\x8D\x0D"
                              "\xCC\xCC\xCC\xCC\xE8\xCC\xCC\xCC\xCC\xF0\x83\x0C\x24\xCC\x48\x8D\x0D\xCC\xCC\xCC\xCC\xE8"
                              "\xCC\xCC\xCC\xCC\xF0\x83\x0C\x24\xCC\xC6\x05",
                              pico::Bool (*)(void*)>();

    Hook::HookBefore<OnReflexFunc>(&OnReflex).OrDie();

    return true;
}