#pragma once
#include <Shared/Pico.hpp>


namespace pico::Engine
{
/**
 * \brief Inspect dynamically resolved imports using guard pages and an exception handler.
 * In production, the KiUserExceptionDispatcher instrumentation callback could be used in place of the exception handler
 * to guarantee first execution.
 */
struct ImportGuard : shared::Util::NonCopyableOrMovable
{
    /**
     * \brief Exception handler.
     * 
     * \param aExceptionPointers Exception information.
     * \return Depending on whether or not the exception should be handled, EXCEPTION_CONTINUE_EXECUTION or EXCEPTION_CONTINUE_SEARCH.
     */
    static LONG OnException(PEXCEPTION_POINTERS aExceptionPointers);
};
}