#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{
/**
 * \brief A wrapper structure for handling instrumentation callback logic.
 * 
 * Instrumentation callbacks are an undocumented Windows mechanic to have a callback run on most kernel mode to user mode transitions.
 * For instance, they get called on most system call returns and thread creation, 
 * as well as on every received exception and user mode APC.
 * 
 * We reset our instrumentation callback every main thread call. 
 * 
 * In theory, this can be improved by the kernel driver checking the EPROCESS structure of our process and validating the callback pointer.
 */
struct InstrumentationCallbacks : public shared::Util::NonCopyableOrMovable
{
    /**
     * \brief Apply the instrumentation callback.
     */
    void TickMainThread() noexcept;

    /**
     * \brief Get an instance of the instrumentation callbacks handler.
     * \return A singleton instance of the instrumentation callbacks handler.
     */
    static InstrumentationCallbacks& Get() noexcept;
};
}