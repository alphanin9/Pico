#pragma once
#include <Shared/Pico.hpp>

#include <asmjit/asmjit.h>

namespace pico::Engine
{
struct InstrumentationCallbackNewThreadRecord
{
      
};

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
    static constexpr asmjit::JitAllocator::CreateParams JitParams = {
        .options = asmjit::JitAllocatorOptions::kFillUnusedMemory | asmjit::JitAllocatorOptions::kCustomFillPattern |
                   asmjit::JitAllocatorOptions::kUseDualMapping,
        .fillPattern = 0x51C0
    };

    pico::Bool m_isInitialized{};

    asmjit::JitRuntime m_jit{ &JitParams };
    asmjit::CodeHolder m_codeHolder{};

    void* m_callback{};

    void AssembleInstrumentationCallback(asmjit::x86::Assembler& aAssembler) noexcept;

    /**
     * \brief Assemble the instrumentation callback assembly stub at runtime.
     */
    void SetupInstrumentationCallback() noexcept;

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