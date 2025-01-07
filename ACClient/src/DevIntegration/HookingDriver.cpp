#include <DevIntegration/HookingDriver.hpp>
#include <DevIntegration/SafetyHookDriver.hpp>

pico::Integration::HookingDriver* pico::Integration::HookingDriver::GetDefault() noexcept
{
    static SafetyHookDriver s_driver{};

    return &s_driver;
}