#pragma once
#include <DevIntegration/HookingDriver.hpp>
#include <safetyhook/easy.hpp>

namespace pico::Integration
{
struct SafetyHookDriver : public Integration::HookingDriver
{
    pico::Bool Attach(uintptr_t aAddy, void* aDetour) override;
    pico::Bool Attach(uintptr_t aAddy, void* aDetour, void** aOriginal) override;
    pico::Bool Detach(uintptr_t aAddy) override;

    pico::Size GetPatchSize(uintptr_t aAddy) override;

    // Key-value pair map of hook address - hook info
    // So we don't end up detecting our own patches by accident
    pico::HashMap<uintptr_t, safetyhook::InlineHook> m_hookDataMap{};
};
} // namespace pico::Integration