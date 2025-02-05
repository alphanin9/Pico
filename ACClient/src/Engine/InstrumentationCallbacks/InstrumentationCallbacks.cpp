#include <Engine/Engine.hpp>
#include <Engine/InstrumentationCallbacks/InstrumentationCallbacks.hpp>
#include <Engine/Logging/Logger.hpp>

#include <lazy_importer.hpp>

/*
 * A small note on instrumentation callback handler functions:
 * Either we use the handy stuff in TEB to avoid re-entrancy or write code in a way that it's never re-entrant where it
 * harms us
 * We elect to do the second, so some things may be restricted (not too many, though!)
 */

namespace Detail
{
struct AsmjitErrorReporter : public asmjit::ErrorHandler
{
public:
    void handleError(asmjit::Error aErr, const char* aMessage, asmjit::BaseEmitter* aOrigin) override
    {
        pico::Engine::Logger::GetLogSink()->error("[Asmjit] Error {} ({})! Message {} from emitter {}", aErr,
                                                  asmjit::DebugUtils::errorAsString(aErr), aMessage,
                                                  reinterpret_cast<void*>(aOrigin));
    }
};

/**
 * \brief Note: this is NECESSARY because TLS WILL fuck you up if you get callback in LdrInitializeThunk
 */
pico::Engine::InstrumentationCallbacks* m_globalPtr{};
} // namespace Detail

void pico::Engine::InstrumentationCallbacks::AssembleInstrumentationCallback(
    asmjit::x86::Assembler& aAssembler) noexcept
{
    // While we could just import them, I'm not sure it'd work properly
    // This does
    static void* RtlCaptureContext = LI_FN(RtlCaptureContext).nt_cached();
    static void* RtlRestoreContext = LI_FN(RtlRestoreContext).nt_cached();
    static void* LdrInitializeThunk = LI_FN(LdrInitializeThunk).nt_cached();
    static void* KiUserExceptionDispatcher = LI_FN(KiUserExceptionDispatcher).nt_cached();
    static void* KiUserApcDispatcher = LI_FN(KiUserApcDispatcher).nt_cached();

    static constexpr auto ContextRip = asmjit::x86::qword_ptr(asmjit::x86::rcx, offsetof(Windows::CONTEXT, Rip));
    static constexpr auto ContextRcx = asmjit::x86::qword_ptr(asmjit::x86::rcx, offsetof(Windows::CONTEXT, Rcx));
    static constexpr auto ContextRsp = asmjit::x86::qword_ptr(asmjit::x86::rcx, offsetof(Windows::CONTEXT, Rsp));

    // Note: we only make any special logic for the important ones
    // Because they're, well, *important*
    const auto labelLdr = aAssembler.newNamedLabel("OnLdrInitializeThunk");
    const auto labelException = aAssembler.newNamedLabel("OnKiUserExceptionDispatcher");
    const auto labelApc = aAssembler.newNamedLabel("OnKiUserApcDispatcher");
    const auto labelExit = aAssembler.newNamedLabel("OnExitCallback");

    // Source RIP is in R10
    // Backup stack pointer and RIP
    auto GsPc = asmjit::x86::qword_ptr_abs(offsetof(Windows::TEB, InstrumentationCallbackPreviousPc));
    GsPc.setSegment(asmjit::x86::gs);

    auto GsSp = asmjit::x86::qword_ptr_abs(offsetof(Windows::TEB, InstrumentationCallbackPreviousSp));
    GsSp.setSegment(asmjit::x86::gs);

    aAssembler.mov(GsPc, asmjit::x86::r10);
    aAssembler.mov(GsSp, asmjit::x86::rsp);

    // Make stack space for the context we'll restore to after our handler, align stack, capture ctx
    aAssembler.sub(asmjit::x86::rsp, sizeof(Windows::CONTEXT));
    aAssembler.and_(asmjit::x86::rsp, -16ull);
    // Back the original RCX up, set up RCX for call
    aAssembler.mov(asmjit::x86::r11, asmjit::x86::rcx);
    aAssembler.mov(asmjit::x86::rcx, asmjit::x86::rsp);

    // Imports mess with us, so we hit back with baffling memes
    // We're also using actual exports from NTDLL instead of imports now
    aAssembler.mov(asmjit::x86::r10, asmjit::imm(reinterpret_cast<pico::Uint64>(RtlCaptureContext)));
    aAssembler.call(asmjit::x86::r10);

    // After this we can do whatever we want to regs
    // Backup context to be available in R15 if we want to use it
    aAssembler.mov(asmjit::x86::r15, asmjit::x86::rcx);
    aAssembler.mov(asmjit::x86::r10, GsPc);

    aAssembler.mov(ContextRip, asmjit::x86::r10);
    aAssembler.mov(ContextRcx, asmjit::x86::r11);

    // R11 is useless now, scratch it

    aAssembler.mov(asmjit::x86::r11, GsSp);
    aAssembler.mov(ContextRsp, asmjit::x86::r11);

    // Now that we have a backup context, we can start doing actual IC logic
    // Note: to avoid errors, we need to move to temp reg first
    aAssembler.mov(asmjit::x86::r11, reinterpret_cast<pico::Uint64>(LdrInitializeThunk));
    aAssembler.cmp(asmjit::x86::r10, asmjit::x86::r11);
    aAssembler.jz(labelLdr);
    aAssembler.mov(asmjit::x86::r11, reinterpret_cast<pico::Uint64>(KiUserExceptionDispatcher));
    aAssembler.cmp(asmjit::x86::r10, asmjit::x86::r11);
    aAssembler.jz(labelException);
    aAssembler.mov(asmjit::x86::r11, reinterpret_cast<pico::Uint64>(KiUserApcDispatcher));
    aAssembler.cmp(asmjit::x86::r10, asmjit::x86::r11);
    aAssembler.jz(labelApc);

    // Start generic call
    aAssembler.mov(asmjit::x86::r10, reinterpret_cast<pico::Uint64>(&OnUnknownInstrumentationCallback));
    aAssembler.sub(asmjit::x86::rsp, 32u);
    aAssembler.call(asmjit::x86::r10);
    aAssembler.jmp(labelExit);
    // End generic call

    // Start OnLdrInitializeThunk
    aAssembler.bind(labelLdr);
    aAssembler.mov(asmjit::x86::rcx, ContextRcx);
    aAssembler.mov(asmjit::x86::r10, reinterpret_cast<pico::Uint64>(&OnLdrInitializeThunk));
    aAssembler.sub(asmjit::x86::rsp, 32u);
    aAssembler.call(asmjit::x86::r10);
    aAssembler.jmp(labelExit);
    // End OnLdrInitializeThunk

    // Start OnKiUserExceptionDispatcher
    aAssembler.bind(labelException);
    aAssembler.mov(asmjit::x86::rcx, ContextRsp);
    aAssembler.mov(asmjit::x86::rdx, asmjit::x86::rcx);
    // IDK what the additional 32 bytes are
    aAssembler.add(asmjit::x86::rcx, sizeof(Windows::CONTEXT) + 32u);

    aAssembler.mov(asmjit::x86::r10, reinterpret_cast<pico::Uint64>(&OnKiUserExceptionDispatcher));
    aAssembler.sub(asmjit::x86::rsp, 32u);
    aAssembler.call(asmjit::x86::r10);
    aAssembler.jmp(labelExit);
    // End OnKiUserExceptionDispatcher

    // Start OnKiUserApcDispatcher
    aAssembler.bind(labelApc);
    aAssembler.mov(asmjit::x86::r10, reinterpret_cast<pico::Uint64>(&OnKiUserApcDispatcher));
    aAssembler.sub(asmjit::x86::rsp, 32u);
    aAssembler.call(asmjit::x86::r10);
    aAssembler.jmp(labelExit);
    // End OnKiUserApcDispatcher

    // Restore context and subsequently exit
    aAssembler.bind(labelExit);
    aAssembler.mov(asmjit::x86::rcx, asmjit::x86::r15);
    aAssembler.mov(asmjit::x86::rdx, 0u);
    aAssembler.sub(asmjit::x86::rsp, 32u);
    aAssembler.mov(asmjit::x86::r10, asmjit::imm(reinterpret_cast<pico::Uint64>(RtlRestoreContext)));
    aAssembler.call(asmjit::x86::r10);

    // WTF?
    aAssembler.int3();
}

void pico::Engine::InstrumentationCallbacks::SetupInstrumentationCallback() noexcept
{
    // Set the code holder up
    m_codeHolder.init(m_jit.environment(), m_jit.cpuFeatures());

    asmjit::StringLogger logger{};

    Detail::AsmjitErrorReporter reporter{};

    m_codeHolder.setErrorHandler(&reporter);
    m_codeHolder.setLogger(&logger);

    // Unfortunately, the assembler is single use
    asmjit::x86::Assembler assembler(&m_codeHolder);

    assembler.setErrorHandler(&reporter);

    // Assemble
    AssembleInstrumentationCallback(assembler);

    const auto err = m_jit.add(&m_callback, &m_codeHolder);

    // Note: callback asm code should also be hashed and periodically compared
    // Maybe regenerate callback once in a while as well?
    Logger::GetLogSink()->info("[Asmjit] JIT stub: \n{}", logger.data());

    if (err)
    {
        Logger::GetLogSink()->error("[Asmjit] Failed to assemble instrumentation callback! Error: {}",
                                    asmjit::DebugUtils::errorAsString(err));
        return;
    }

    Logger::GetLogSink()->info("[Instrumentation] Instrumentation callback placed at {}", m_callback);
}

void pico::Engine::InstrumentationCallbacks::TickMainThread() noexcept
{
    if (!m_isInitialized)
    {
        // This is safe because we're a singleton anyway
        Detail::m_globalPtr = this;
        SetupInstrumentationCallback();
        m_isInitialized = true;
    }

    if (m_callback)
    {
        Windows::PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION info{};

        info.Callback = m_callback;

        Windows::NtSetInformationProcess(GetCurrentProcess(), Windows::PROCESSINFOCLASS::ProcessInstrumentationCallback,
                                         &info, sizeof(info));
    }
}

void pico::Engine::InstrumentationCallbacks::OnLdrInitializeThunk(pico::Uint64 aThreadStartAddress) noexcept
{
    NewThreadRecord record{.m_threadStartAddress = aThreadStartAddress, .m_threadId = shared::ProcessEnv::GetTID()};

    std::lock_guard _(Detail::m_globalPtr->m_newThreadLock);

    if (Detail::m_globalPtr->m_newThreadRecordCount >= Detail::m_globalPtr->m_newThreadRecords.size())
    {
        return;
    }

    Detail::m_globalPtr->m_newThreadRecords[Detail::m_globalPtr->m_newThreadRecordCount] = record;
    Detail::m_globalPtr->m_newThreadRecordCount++;
}

void pico::Engine::InstrumentationCallbacks::OnKiUserExceptionDispatcher(Windows::EXCEPTION_RECORD* aRecord,
                                                                         Windows::CONTEXT* aContext) noexcept
{
    ExceptionRecord record{.m_record = *aRecord, .m_threadId = shared::ProcessEnv::GetTID(), .m_rip = aContext->Rip};

    auto& engine = Engine::Get();

    const auto pageLow = shared::Util::AlignDown(aContext->Rsp, static_cast<pico::Uint64>(engine.m_pageSize));

    record.m_stackPage.assign(engine.m_pageSize / sizeof(void*), {});
    std::copy_n(reinterpret_cast<void**>(pageLow), record.m_stackPage.size(), record.m_stackPage.begin());

    std::lock_guard _(Detail::m_globalPtr->m_exceptionLock);
    Detail::m_globalPtr->m_exceptionRecords.push_back(std::move(record));
}

void pico::Engine::InstrumentationCallbacks::OnKiUserApcDispatcher(Windows::CONTEXT*) noexcept
{
    // Currently ignored
}

void pico::Engine::InstrumentationCallbacks::OnUnknownInstrumentationCallback(Windows::CONTEXT*) noexcept
{
    // Currently ignored
}

pico::Engine::InstrumentationCallbacks& pico::Engine::InstrumentationCallbacks::Get() noexcept
{
    static InstrumentationCallbacks s_instance{};

    return s_instance;
}
