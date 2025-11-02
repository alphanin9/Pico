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
 * 
 * NOTE: currently instrumentation callback is JIT-generated. 
 * This isn't optimal, but it's a XMake Moment:tm: where it doesn't add .asm files to the VS2022 Solution Explorer.
 */
struct InstrumentationCallbacks : public shared::Util::NonCopyableOrMovable
{
    /**
     * \brief An internal structure describing a new thread. Recorded upon LdrInitializeThunk.
     */
    struct NewThreadRecord
    {
        void* m_threadStartAddress{};
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
        .fillPattern = 0x51C051C0};

    pico::Bool m_isInitialized{};

    // JIT runtime, needs special treatment for cleanup
    asmjit::JitRuntime m_jit{&JitParams};

    // Code holder for instrumentation callback logic
    asmjit::CodeHolder m_callbackCodeHolder{};

    // Pointer to assembled callback
    void* m_callback{};

    // Size of assembled callback for detection functions
    pico::Size m_sizeOfCallback{};

    // Neither should cause reentrancy, so we're fine!
    // You can't do stuff before TLS kicks in, so no - we're not fine!
    std::mutex m_newThreadLock{};
    pico::Int32 m_newThreadRecordCount{};
    std::array<NewThreadRecord, 1024u> m_newThreadRecords{};

    std::mutex m_exceptionLock{};
    pico::Vector<ExceptionRecord> m_exceptionRecords{};

    /**
     * \brief Assembles the instrumentation callback's JIT stub.
     * \param aAssembler The assembler the function will use to emit instructions.
     */
    void AssembleInstrumentationCallback(asmjit::x86::Assembler& aAssembler);

    /**
     * \brief Setup the instrumentation callback stub.
     */
    void SetupInstrumentationCallback();

    /**
     * \brief Apply the instrumentation callback. If not initialized yet, assemble the instrumentation callback.
     */
    void TickMainThread();

    /**
     * \brief Ticks the component in the thread pool. Analyzes new results of the callbacks.
     */
    void Tick();

    /**
     * \brief Analyzes the records of new threads created.
     */
    void UpdateThreads();

    /**
     * \brief Analyzes the records of new exceptions.
     */
    void UpdateExceptions();

    /**
     * \brief Attempts to disable the instrumentation callback. This is necessary, as otherwise the protected
     * application will have interesting crashes during exit.
     */
    void Teardown();

    /**
     * \brief Checks whether or not unbacked code belongs to our instrumentation callback.
     * \param aRip The instruction pointer.
     * \return Whether or not the code belongs to us.
     */
    pico::Bool IsCodeInInstrumentationCallback(uintptr_t aRip) const;

    /**
     * \brief Checks if the given allocation has been made by our JIT allocator.
     *
     * \param aAddress The address of the presumed allocation.
     * \return Whether or not the allocation is owned by our allocator.
     */
    pico::Bool IsAddressAllocatedByJITAllocator(uintptr_t aAddress) const;

    /**
     * \brief Get an instance of the instrumentation callbacks handler.
     * \return A singleton instance of the instrumentation callbacks handler.
     */
    static InstrumentationCallbacks& Get();

    // Callback handlers

    /**
     * \brief Instrumentation callback for new threads created in our process context. No particular re-entrancy
     * constraints. Code has to not use TLS.
     * \param aThreadStartAddress The start address of the new thread.
     */
    static void OnLdrInitializeThunk(void* aThreadStartAddress);

    /**
     * \brief Instrumentation callback for exceptions raised in our process context. No particular re-entrancy
     * constraints.
     * \param aExceptionInfo The exception record describing the exception.
     * \param aContext The thread context at the moment of the exception.
     */
    static void OnKiUserExceptionDispatcher(Windows::EXCEPTION_RECORD* aExceptionInfo, Windows::CONTEXT* aContext);

    /**
     * \brief Instrumentation callback for APCs received for our process. Unknown re-entrancy constraints.
     *
     * Can be used to dump thread stacks with QueueUserAPC2.
     *
     * \param aApcReceiver The function intended to receive the APC.
     * \param aApcData The APC context structure built by Windows. Only valid when using our APC receiver function.
     */
    static void OnKiUserApcDispatcher(void* aApcReceiver, APC_CALLBACK_DATA* aApcData);

    /**
     * \brief Instrumentation callback for unknown calls (no specific handler). Can be a system call or something we
     * missed. Cannot do more system calls.
     * \param aCtx The instrumentation callback's acquired context.
     */
    static void OnUnknownInstrumentationCallback(Windows::CONTEXT* aCtx);
};
} // namespace pico::Engine