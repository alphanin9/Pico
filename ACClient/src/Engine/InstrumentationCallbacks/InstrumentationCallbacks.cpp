#include <Engine/InstrumentationCallbacks/InstrumentationCallbacks.hpp>
#include <Engine/Logging/Logger.hpp>
#include <lazy_importer.hpp>

/*
 * A small note on instrumentation callback handler functions:
 * Either we use the handy stuff in TEB to avoid re-entrancy or write code in a way that it's never re-entrant
 * We elect to do the second, so some things may be restricted (not too many, though)
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
} // namespace Detail

void pico::Engine::InstrumentationCallbacks::AssembleInstrumentationCallback(
    asmjit::x86::Assembler& aAssembler) noexcept
{
    auto captureContext = LI_FN(RtlCaptureContext).nt_cached<void*>();
    auto restoreContext = LI_FN(RtlRestoreContext).nt_cached<void*>();

    // Source RIP is in R10
    // Backup stack pointer and RIP
    auto gsPc = asmjit::x86::qword_ptr_abs(offsetof(Windows::TEB, InstrumentationCallbackPreviousPc));
    gsPc.setSegment(asmjit::x86::gs);

    auto gsSp = asmjit::x86::qword_ptr_abs(offsetof(Windows::TEB, InstrumentationCallbackPreviousSp));
    gsSp.setSegment(asmjit::x86::gs);

    aAssembler.mov(gsPc, asmjit::x86::r10);
    aAssembler.mov(gsSp, asmjit::x86::rsp);

    // Make stack space for the context we'll restore to after our handler, align stack, capture ctx
    aAssembler.sub(asmjit::x86::rsp, sizeof(Windows::CONTEXT));
    aAssembler.and_(asmjit::x86::rsp, -16ull);
    // Back the original RCX up, set up RCX for call
    aAssembler.mov(asmjit::x86::r11, asmjit::x86::rcx);
    aAssembler.mov(asmjit::x86::rcx, asmjit::x86::rsp);

    // Imports mess with us, so we hit back with baffling memes
    // We're also using actual exports from NTDLL instead of imports now
    aAssembler.mov(asmjit::x86::r10, asmjit::imm(reinterpret_cast<pico::Uint64>(captureContext)));
    aAssembler.call(asmjit::x86::r10);

    const auto contextRip = asmjit::x86::qword_ptr(asmjit::x86::rcx, offsetof(Windows::CONTEXT, Rip));
    const auto contextRcx = asmjit::x86::qword_ptr(asmjit::x86::rcx, offsetof(Windows::CONTEXT, Rcx));
    const auto contextRsp = asmjit::x86::qword_ptr(asmjit::x86::rcx, offsetof(Windows::CONTEXT, Rsp));

    aAssembler.mov(asmjit::x86::r10, gsPc);

    aAssembler.mov(contextRip, asmjit::x86::r10);
    aAssembler.mov(contextRcx, asmjit::x86::r11);

    // R11 is useless now, scratch it

    aAssembler.mov(asmjit::x86::r11, gsSp);
    aAssembler.mov(contextRsp, asmjit::x86::r11);
    // Now that we have a backup context, we can start doing actual IC logic

    // For testing, we just restore context immediately tho
    // RCX is still context

    aAssembler.mov(asmjit::x86::rdx, 0u);
    // Setup stack shadow space
    aAssembler.sub(asmjit::x86::rsp, 32u);

    
    // Go back
    aAssembler.mov(asmjit::x86::r10, asmjit::imm(reinterpret_cast<pico::Uint64>(restoreContext)));
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

    Logger::GetLogSink()->info("[Asmjit] Logs: {}", logger.data());

    if (err)
    {
        Logger::GetLogSink()->error("Failed to assemble instrumentation callback! Error: {}",
                                    asmjit::DebugUtils::errorAsString(err));
        return;
    }

    Logger::GetLogSink()->info("Instrumentation callback placed at {}", m_callback);
}

void pico::Engine::InstrumentationCallbacks::TickMainThread() noexcept
{
    if (!m_isInitialized)
    {
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

pico::Engine::InstrumentationCallbacks& pico::Engine::InstrumentationCallbacks::Get() noexcept
{
    static InstrumentationCallbacks s_instance{};

    return s_instance;
}
