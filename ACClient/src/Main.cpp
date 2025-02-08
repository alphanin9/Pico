#include <API/APIFunctions.hpp>
#include <DevIntegration/CS2/CS2.hpp>
#include <DevIntegration/Integration.hpp>
#include <DevIntegration/Rust/Rust.hpp>
#include <Shared/Pico.hpp>

namespace pico
{
/**
 * \brief Initializes dev purpose integration via hooking
 *
 * NOTE: In real Production Environment:tm:, this would be handled by the game calling SDK init
 * + game main loop polling API every now and then
 * + user component being loaded in by kernel component upon process start
 *
 * Because we do not have access to the protected app's source code + I'm not writing a
 * kernel component for this we can't do initialization or integration properly
 *
 * So we just hook an important function in the game's main loop and bootstrap + poll main loop
 * there
 *
 * In CS2's case, we pick a function in the rendering API that handles the sleep
 * before/after main loop used for lowering input latency
 *
 * This should minimize the performance impact of the anti-cheat main loop callback
 *
 * In a real Production Environment:tm: we also wouldn't really need DllMain
 */
pico::Bool InitializeIntegration()
{
    auto& ctx = Integration::Context::Get();

    ctx.m_logger->info("Pico client is loading...");

    // Are we in CS2?
    if (Integration::IsCS2())
    {
        ctx.m_logger->info("Waiting for render system DLL to be loaded...");
        // If so, wait until one of our detoured modules finishes loading
        Integration::WaitUntilOneOfModulesLoaded({"rendersystemdx11.dll", "rendersystemvulkan.dll"});

        ctx.m_logger->info("Render system module is loaded, starting hooking...");

        return pico::Integration::CS2::InitCS2();
    }

    else if (Integration::IsRust())
    {
        ctx.m_logger->info("Waiting for game assembly to be loaded...");
        Integration::WaitUntilModuleLoaded("GameAssembly.dll");

        ctx.m_logger->info("GameAssembly is loaded, starting hooking...");

        return pico::Integration::Rust::InitRust();
    }

    ctx.m_logger->error("Running in unknown executable, Pico integration routine is meant to be used in CS2/Rust!");

    return false;
}

pico::Uint32 InitThread(void* aParam)
{
    return !InitializeIntegration();
}
} // namespace pico

BOOL __stdcall DllMain(HMODULE aModule, pico::Uint32 aReason, void* aReserved)
{
    DisableThreadLibraryCalls(aModule);
    switch (aReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        // Initialize our integration process
        if (const auto handle = CreateThread(nullptr, 0u, reinterpret_cast<LPTHREAD_START_ROUTINE>(&pico::InitThread),
                                             nullptr, 0u, nullptr))
        {
            CloseHandle(handle);
        }
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        Pico_Teardown();
        break;
    }
    default:
        break;
    }

    return TRUE;
}