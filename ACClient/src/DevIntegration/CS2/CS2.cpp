#include <DevIntegration/CS2/CS2.hpp>
#include <DevIntegration/Integration.hpp>

#include <API/APIFunctions.hpp>

void pico::Integration::CS2::OnReflexDX11(void* aRcx)
{
    static auto s_initialized = false;

    if (!s_initialized)
    {
        Pico_Init();
        s_initialized = true;
    }

    Pico_PollMainLoop();
}

void pico::Integration::CS2::OnReflexVulkan(void* aRcx)
{
    static auto s_initialized = false;

    if (!s_initialized)
    {
        Pico_Init();
        s_initialized = true;
    }

    Pico_PollMainLoop();
}

pico::Bool pico::Integration::CS2::InitCS2() noexcept
{
    const auto isDX11 = static_cast<pico::Bool>(shared::ProcessEnv::GetModuleByName("rendersystemdx11.dll"));
    const auto isVulkan = static_cast<pico::Bool>(shared::ProcessEnv::GetModuleByName("rendersystemvulkan.dll"));

    if (isDX11)
    {
        constexpr auto OnReflexDX11Func =
            pico::shared::RawFunc<"rendersystemdx11.dll", "\x40\x55\x53\x41\x54\x41\x56", pico::Int64 (*)(void*)>();

        Hook::HookBefore<OnReflexDX11Func>(&OnReflexDX11).OrDie();

        return true;
    }
    else if (isVulkan)
    {
        constexpr auto OnReflexVulkanFunc =
            pico::shared::RawFunc<"rendersystemvulkan.dll",
                                  "\x48\x89\x5C\x24\xCC\x55\x56\x57\x41\x56\x41\x57\x48\x8D\x6C\x24\xCC\x48\x81\xEC\xCC"
                                  "\xCC\xCC\xCC\x48\x8B\xF9",
                                  pico::Int64 (*)(void*)>();

        Hook::HookBefore<OnReflexVulkanFunc>(&OnReflexVulkan).OrDie();

        return true;
    }

    return false;
}
