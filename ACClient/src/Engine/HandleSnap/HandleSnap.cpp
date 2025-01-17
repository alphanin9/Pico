#include <Engine/HandleSnap/HandleSnap.hpp>
#include <Engine/Logging/Logger.hpp>
void pico::Engine::HandleSnap::Tick() noexcept
{
    constexpr pico::Seconds CheckDuration{5};
    const auto now = Clock::now();

    if (now - m_lastCheckTime <= CheckDuration)
    {
        return;
    }

    m_lastCheckTime = now;

    static const auto s_pid = static_cast<pico::Uint32>(shared::ProcessEnv::GetCurrentThreadEnvironment()->ClientId.UniqueProcess);

    shared::SystemEnv::EnumerateHandles(
        [](const Windows::SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX& aEntry)
        {
            // Hack, we should resolve this dynamically
            constexpr auto ObjectTypeProcess = 8u;

            auto& logger = Logger::GetLogSink();

            // Note: should check for thread handles to us too, but later
            if (aEntry.ObjectTypeIndex != ObjectTypeProcess)
            {
                return;
            }

            // Handle doesn't do much
            if ((aEntry.GrantedAccess & PROCESS_CREATE_THREAD) == 0u &&
                (aEntry.GrantedAccess & PROCESS_VM_OPERATION) == 0u && (aEntry.GrantedAccess & PROCESS_VM_READ) == 0u &&
                (aEntry.GrantedAccess & PROCESS_VM_WRITE) == 0u)
            {
                return;
            }

            const auto pid = reinterpret_cast<pico::Uint32>(aEntry.UniqueProcessId);

            // Open a handle, if we can't - well we're screwed
            // Nonetheless we should take a snapshot of all running processes every now and then as well, dump some
            // stuff about them and all
            wil::unique_handle procHandle{OpenProcess(reinterpret_cast<DWORD>(aEntry.UniqueProcessId), false,
                                                      PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_DUP_HANDLE)};

            // We can't query even limited info about the process from here
            if (!procHandle.is_valid())
            {
                logger->warn("Failed to open a handle to PID {}", pid);
            }

            wil::unique_handle dupHandle{};

            if (!NT_SUCCESS(Windows::NtDuplicateObject(procHandle.get(), aEntry.HandleValue, GetCurrentProcess(), *dupHandle.addressof(), PROCESS_QUERY_LIMITED_INFORMATION, 0u, 0u))) {
                logger->warn("Failed to duplicate handle from PID {}", pid);
                return;
            }

            const auto targetPid = GetProcessId(dupHandle.get());

            if (targetPid == s_pid)
            {
                pico::UnicodeString procName{};
                wil::QueryFullProcessImageNameW(procHandle.get(), PROCESS_NAME_NATIVE, procName);

                logger->warn("Process {} has a potentially dangerous handle open to us!",
                             shared::Util::ToUTF8(procName));
            }
        });
}

pico::Engine::HandleSnap& pico::Engine::HandleSnap::Get() noexcept
{
    static HandleSnap s_instance{};

    return s_instance;
}
