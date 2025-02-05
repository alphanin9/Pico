#pragma once
#include <Shared/Pico.hpp>

#include <asmjit/asmjit.h>

namespace pico::Engine
{
/**
 * \brief A wrapper structure for handling instrumentation callback logic.
 *
 * Instrumentation callbacks are an undocumented Windows mechanic to have a callback run on most kernel mode to user
 * mode transitions. For instance, they get called on most system call returns and thread creation, as well as on every
 * received exception and user mode APC.
 *
 * We reset our instrumentation callback every main thread call.
 *
 * In theory, this can be improved by the kernel driver checking the EPROCESS structure of our process and validating
 * the callback pointer.
 */
struct InstrumentationCallbacks : public shared::Util::NonCopyableOrMovable
{
    /**
     * \brief An internal structure describing a new thread. Recorded upon LdrInitializeThunk.
     */
    struct NewThreadRecord
    {
        pico::Uint64 m_threadStartAddress{};
        pico::Uint32 m_threadId{};
    };

    /**
     * \brief An internal structure describing an exception. Recorded upon KiUserExceptionDispatcher.
     */
    struct ExceptionRecord
    {
        Windows::EXCEPTION_RECORD m_record{};

        pico::Uint32 m_threadId{};
        pico::Uint64 m_rip{};
        pico::Vector<void*> m_stackPage{};
    };

    /**
     * \brief An internal structure describing a generic callback result. Recorded upon an instrumentation callback with
     * no dedicated handler. This is currently unused due to performance and concurrency concerns.
     */
    struct GenericCallbackRecord
    {
        pico::Uint64 m_rip{};
        pico::Uint32 m_threadId{};

        std::array<void*, 0x200 / sizeof(void*)> m_stack{};
    };

    static constexpr asmjit::JitAllocator::CreateParams JitParams = {
        .options = asmjit::JitAllocatorOptions::kFillUnusedMemory | asmjit::JitAllocatorOptions::kCustomFillPattern |
                   asmjit::JitAllocatorOptions::kUseDualMapping,
        .fillPattern = 0x51C0};

    pico::Bool m_isInitialized{};

    asmjit::JitRuntime m_jit{&JitParams};
    asmjit::CodeHolder m_codeHolder{};

    void* m_callback{};

    // Neither should cause reentrancy, so we're fine!
    // You can't do stuff before TLS kicks in, so no - we're not fine!
    std::mutex m_newThreadLock{};
    pico::Int32 m_newThreadRecordCount{};
    std::array<NewThreadRecord, 1024u> m_newThreadRecords{};

    std::mutex m_exceptionLock{};
    pico::Vector<ExceptionRecord> m_exceptionRecords{};

    /**
     * \brief Assembles the instrumentation callback's JIT stub.
     *
     * \param aAssembler The assembler the function will use to emit instructions.
     */
    void AssembleInstrumentationCallback(asmjit::x86::Assembler& aAssembler) noexcept;

    /**
     * \brief Setup the instrumentation callback stub.
     */
    void SetupInstrumentationCallback() noexcept;

    /**
     * \brief Apply the instrumentation callback. If not initialized yet, assemble the instrumentation callback.
     */
    void TickMainThread() noexcept;

    /**
     * \brief Get an instance of the instrumentation callbacks handler.
     * \return A singleton instance of the instrumentation callbacks handler.
     */
    static InstrumentationCallbacks& Get() noexcept;

    // Callback handlers

    /**
     * \brief Instrumentation callback for new threads created in our process context. No particular re-entrancy
     * constraints. Code has to not use TLS.
     * \param aThreadStartAddress The start address of the new thread.
     */
    static void OnLdrInitializeThunk(pico::Uint64 aThreadStartAddress) noexcept;

    /**
     * \brief Instrumentation callback for exceptions raised in our process context. No particular re-entrancy
     * constraints.
     * \param aExceptionInfo The exception record describing the exception.
     * \param aContext The thread context at the moment of the exception.
     */
    static void OnKiUserExceptionDispatcher(Windows::EXCEPTION_RECORD* aExceptionInfo,
                                            Windows::CONTEXT* aContext) noexcept;

    /**
     * \brief Instrumentation callback for APCs received for our process.
     * \param aCtx The instrumentation callback's acquired context.
     */
    static void OnKiUserApcDispatcher(Windows::CONTEXT* aCtx) noexcept;

    /**
     * \brief Instrumentation callback for unknown calls (no specific handler). Can be a system call or something we
     * missed.
     * \param aCtx The instrumentation callback's acquired context.
     */
    static void OnUnknownInstrumentationCallback(Windows::CONTEXT* aCtx) noexcept;
};
} // namespace pico::Engine