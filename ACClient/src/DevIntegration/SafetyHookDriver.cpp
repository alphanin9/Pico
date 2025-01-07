#include <Shared/Pico.hpp>
#include <DevIntegration/SafetyHookDriver.hpp>
#include <DevIntegration/Context.hpp>

pico::Bool pico::Integration::SafetyHookDriver::Attach(uintptr_t aAddy, void* aDetour) noexcept
{
    Context::Get().m_logger->info("Attempting to attach hook at {:#x}", aAddy);
    auto hook = safetyhook::create_inline(reinterpret_cast<void*>(aAddy), aDetour);

    // We screwed up
    if (!hook)
    {
        return false;
    }

    // Move the hook to the datamap
    m_hookDataMap.insert_or_assign(hook.target_address(), std::move(hook));

    return true;
}

pico::Bool pico::Integration::SafetyHookDriver::Attach(uintptr_t aAddy, void* aDetour, void** aOriginal) noexcept
{
    Context::Get().m_logger->info("Attempting to attach hook at {:#x}", aAddy);
    auto hook = safetyhook::create_inline(reinterpret_cast<void*>(aAddy), aDetour);

    // We screwed up
    if (!hook)
    {
        return false;
    }

    *aOriginal = reinterpret_cast<void*>(hook.trampoline().address());

    // Move the hook to the datamap
    m_hookDataMap.insert_or_assign(hook.target_address(), std::move(hook));

    return true;
}

pico::Bool pico::Integration::SafetyHookDriver::Detach(uintptr_t aAddy) noexcept
{
    if (!m_hookDataMap.contains(aAddy))
    {
        return false;
    }
    
    // Safetyhook inline hook destructor should take care of detaching
    // In theory, we should be holding onto the hook for as long as possible to optimize 
    // But I don't think we will ever be detaching
    m_hookDataMap.erase(aAddy);

    return true;
}

pico::Size pico::Integration::SafetyHookDriver::GetPatchSize(uintptr_t aAddy) noexcept
{
    // Note: assuming we're integrity checking functions 
    // in a sane way (either from start of .text or looking at exception directory), 
    // we should always hit the start of our patch
    if (!m_hookDataMap.contains(aAddy))
    {
        return 0u;
    }

    // I think this should be OK
    return m_hookDataMap[aAddy].original_bytes().size();
}