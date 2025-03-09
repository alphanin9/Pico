#include <Shared/Pico.hpp>
#include <print>

/* NOTE: This launcher is not fit for production code and should only be used for debugging
 * Better execution flow:
 * - Launcher downloads and starts driver, verifies driver is there,
 * - Driver does various system integrity checks
 * - Both launcher and driver have heartbeats to tell the backend "hey, we're here!"
 * - Driver also cross-checks system integrity check results with user-mode launcher
 * - Launcher starts suspended game process, driver has callback/whatever else fire
 * - Driver maps user-mode library component into game process
 * - User-mode library component starts its own heartbeat + routines
 * - Game signals user-mode component (via SDK calling driver somehow? whatevs)
 * - With various data every now and then
 *
 * ...or just implement Epic Online Services, it's far easier and will give you a better result
 */

namespace pico
{
enum class ELaunchGames
{
    CS2,
    Rust,
    Max
};
} // namespace pico

pico::Int32 main(pico::Int32 aArgc, pico::Char** aArgv)
{
    std::println("PicoAC launcher starting...");

    constexpr pico::ELaunchGames DefaultApp = pico::ELaunchGames::CS2;
    auto launchedApp = DefaultApp;

    if (aArgc >= 2)
    {
        // Get first param of command line
        pico::StringView appName = aArgv[1];

        // Hacky, but whatever
        if (appName == "cs2")
        {
            launchedApp = pico::ELaunchGames::CS2;
        }
        else if (appName == "rust")
        {
            launchedApp = pico::ELaunchGames::Rust;
        }
    }

    std::println("Starting app...");

    // Currently we pull launched application path from environment variables
    // We do not perform any error handling here, as this is merely for debugging and dev

    pico::UnicodeString applicationPath{};

    // Note: some applications will also require command line parameters to work correctly
    // in our usecase
    pico::UnicodeString commandLine{};

    // Points to CS2.exe
    constexpr auto CS2PathEnvVar = L"CS2_PATH";

    // Points to RustClient.exe, Rust.exe is EasyAntiCheat's launcher of the game
    constexpr auto RustPathEnvVar = L"RUST_CLIENT_PATH";

    if (launchedApp == pico::ELaunchGames::CS2)
    {
        wil::TryGetEnvironmentVariableW(CS2PathEnvVar, applicationPath);
        // Disable VAC
        commandLine = applicationPath + L" -insecure";
    }
    else
    {
        wil::TryGetEnvironmentVariableW(RustPathEnvVar, applicationPath);
        commandLine = applicationPath;
    }

    wil::unique_process_information procInfo{};

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);

    if (CreateProcessW(applicationPath.c_str(), &commandLine[0], nullptr, nullptr, false, CREATE_SUSPENDED, nullptr,
                       nullptr, &startupInfo, procInfo.addressof()))
    {
        // Note: Should be handled by build system somehow, this is not optimal
        constexpr auto ClientRelativePath = L".\\ACClient.dll";

        pico::UnicodeString fullPath{};
        wil::GetFullPathNameW(ClientRelativePath, fullPath);

        // We do not use proper RAII here for remote alloc (Note: WIL does not provide bindings for VirtualAllocEx),
        // which is a bit of a problem - but whatever

        uintptr_t memoryBaseAddress{};
        pico::Size regionSize = fullPath.size();
        if (!NT_SUCCESS(Windows::NtAllocateVirtualMemory(procInfo.hProcess, memoryBaseAddress, 0u, regionSize,
                                                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)))
        {
            return 1;
        }

        pico::Size bytesWritten{};
        if (!WriteProcessMemory(procInfo.hProcess, (void*)(memoryBaseAddress), fullPath.data(),
                                (fullPath.size() + 1) * sizeof(pico::WChar), &bytesWritten))
        {
            return 1;
        }

        // Start new thread with LoadLibraryW as starting point, classic
        // Note that in CS2 the game overlay renderer will hook LoadLibraryExW
        // (probably for anti-cheat/telemetry, just like they hook VirtualAllocEx and VirtualProtectEx)
        wil::unique_handle threadHandle{CreateRemoteThread(procInfo.hProcess, nullptr, 0u,
                                                           (LPTHREAD_START_ROUTINE)(LoadLibraryW),
                                                           (void*)(memoryBaseAddress), 0u, nullptr)};

        if (!threadHandle.is_valid())
        {
            return 1;
        }

        // Resume application to let all threads init
        ResumeThread(procInfo.hThread);
        wil::handle_wait(threadHandle.get());

        // We're done with loading, free the client's path from memory
        // Everything else will be handled by wil
        pico::Size freeSize{};

        if (!NT_SUCCESS(Windows::NtFreeVirtualMemory(procInfo.hProcess, memoryBaseAddress, freeSize, MEM_RELEASE)))
        {
            return 1;
        }

        std::println("Waiting for process exit...");

        wil::handle_wait(procInfo.hProcess);

        pico::Uint32 exitCode{};

        GetExitCodeProcess(procInfo.hProcess, (LPDWORD)(&exitCode));

        std::println("Process exit code: {:#x}", exitCode);

        if (!NT_SUCCESS(exitCode))
        {
            std::println("Process exited with error! See if you can make a minidump now.");
        }

        return 0;
    }

    return 1;
}