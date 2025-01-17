#include <DevIntegration/Util.hpp>

pico::Bool pico::Integration::IsProcess(pico::shared::HashedString aProcNameHash) noexcept
{
    // Get the module name
    UnicodeString fileName{};
    wil::GetModuleFileNameW(nullptr, fileName);

    UnicodeStringView view = fileName;

    // Evil code and we should really be using std::filesystem::path,
    // but the conversion would be expensive
    const auto pos = view.find_last_of(L'\\');

    if (pos == view.npos)
    {
        return false;
    }

    const auto hash = shared::FNV1a64WideCharStringWithLowerCase(view.substr(pos + 1).data());

    return aProcNameHash == hash;
}

pico::Bool pico::Integration::IsCS2() noexcept
{
    return Integration::IsProcess("cs2.exe");
}

pico::Bool pico::Integration::IsRust() noexcept
{
    return Integration::IsProcess("rustclient.exe");
}

void pico::Integration::WaitUntilModuleLoaded(pico::shared::HashedString aModuleName) noexcept
{
    // Just yield until we're good
    while (shared::ProcessEnv::GetModuleByName(aModuleName) == nullptr)
    {
        std::this_thread::sleep_for(pico::Milliseconds(50));
    }
}

void pico::Integration::WaitUntilOneOfModulesLoaded(
    const pico::Vector<pico::shared::HashedString>& aModuleNames) noexcept
{
    bool hasLoaded = false;

    do
    {
        for (auto i : aModuleNames)
        {
            if (shared::ProcessEnv::GetModuleByName(i))
            {
                hasLoaded = true;
                break;
            }
        }

        if (!hasLoaded)
        {
            std::this_thread::sleep_for(pico::Milliseconds(50));
        }
    } while (!hasLoaded);
}

void pico::Integration::Die(pico::Uint32 aErrorCode) noexcept
{
    DebugBreak();
    TerminateProcess(GetCurrentProcess(), aErrorCode);
}