#include <Engine/InstrumentationCallbacks/InstrumentationCallbacks.hpp>

// Our assembly callback handler
// Note: this could theoretically be runtime-generated with Asmjit/xbyak,
// thus removing the need for MASM
extern "C" void* Pico_InstrumentationCallbacks_Receiver;

void pico::Engine::InstrumentationCallbacks::TickMainThread() noexcept
{
    Windows::PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION info{};

    info.Callback = &Pico_InstrumentationCallbacks_Receiver;

    Windows::NtSetInformationProcess(GetCurrentProcess(), Windows::PROCESSINFOCLASS::ProcessInstrumentationCallback,
                                     &info, sizeof(info));
}

pico::Engine::InstrumentationCallbacks& pico::Engine::InstrumentationCallbacks::Get() noexcept
{
    static InstrumentationCallbacks s_instance{};

    return s_instance;
}
