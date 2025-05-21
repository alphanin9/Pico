#include <Engine/ImportGuard/ImportGuard.hpp>

LONG pico::Engine::ImportGuard::OnException(PEXCEPTION_POINTERS aExceptionInfo)
{
    if (aExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
    {
        // Remove trap flag, reapply broken guard page (how? use TLS to keep track of last guard page 
        // and pray our guard page wasn't hit before TLS initializes?), move along.
        aExceptionInfo->ContextRecord->EFlags &= ~0x100;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    if (aExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_GUARD_PAGE)
    {
        const auto faultAddress = aExceptionInfo->ExceptionRecord->ExceptionInformation[1];
        const auto faultPage = shared::Util::AlignDown(faultAddress, 4096ull);

        // TODO

        // Apply trap flag so we can re-apply guard page protection on next instruction
        aExceptionInfo->ContextRecord->EFlags |= 0x100;
        
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}
