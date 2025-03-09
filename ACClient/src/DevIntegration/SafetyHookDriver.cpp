#include <DevIntegration/Context.hpp>
#include <DevIntegration/SafetyHookDriver.hpp>
#include <Shared/Pico.hpp>

pico::Bool pico::Integration::SafetyHookDriver::Attach(uintptr_t aAddy, void* aDetour)
{
    Context::Get().m_logger->info("Attempting to attach hook at {:#x}", aAddy);
    auto hook = safetyhook::create_inline((void*)aAddy, aDetour);

    // We screwed up
    if (!hook)
    {
        return false;
    }

    // Move the hook to the datamap
    m_hookDataMap.insert_or_assign(hook.target_address(), std::move(hook));

    return true;
}

pico::Bool pico::Integration::SafetyHookDriver::Attach(uintptr_t aAddy, void* aDetour, void** aOriginal)
{
    Context::Get().m_logger->info("Attempting to attach hook at {:#x}", aAddy);
    auto hook = safetyhook::create_inline((void*)aAddy, aDetour);

    // We screwed up
    if (!hook)
    {
        return false;
    }

    *aOriginal = (void*)hook.trampoline().address();

    // Move the hook to the datamap
    m_hookDataMap.insert_or_assign(hook.target_address(), std::move(hook));

    return true;
}

pico::Bool pico::Integration::SafetyHookDriver::Detach(uintptr_t aAddy)
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

pico::Size pico::Integration::SafetyHookDriver::GetPatchSize(uintptr_t aAddy)
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