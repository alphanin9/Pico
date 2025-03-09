#include <Shared/EnvironmentIntegrity/SecureBoot.hpp>

pico::shared::EnvironmentIntegrity::SecureBootConfig pico::shared::EnvironmentIntegrity::GetSecureBootConfig()
{
    SecureBootConfig config{};
    pico::Uint32 sizeWritten{};

    Windows::NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS::SystemSecureBootInformation, &config,
                                      sizeof(config), sizeWritten);

    return config;
}

pico::shared::EnvironmentIntegrity::MeasuredBootData pico::shared::EnvironmentIntegrity::GetLastMeasuredBootData()
{
    // This implementation is very, very hacky and can be broken if a hack manually writes crap to measured boot results
    // Does the kernel store results somewhere as well?
    // Should we be storing the whole folder's data?

    pico::UnicodeString searchQuery{};
    wil::GetWindowsDirectoryW(searchQuery);

    // Hack!
    searchQuery += L"\\Logs\\MeasuredBoot\\*";

    WIN32_FIND_DATAW findData{};
    wil::unique_hfind findIterator{FindFirstFileW(searchQuery.c_str(), &findData)};

    if (!findIterator.is_valid())
    {
        return {};
    }

    // Hacky 3AM code
    // These could be static, we know the maximum file name size!
    std::array<pico::WChar, MAX_PATH> latestRawBootLogFileName{};
    pico::Uint64 latestRawBootLogCreateTime{};

    std::array<pico::WChar, MAX_PATH> latestJSONBootLogFileName{};
    pico::Uint64 latestJSONBootLogCreateTime{};

    do
    {
        const pico::UnicodeStringView nameView{findData.cFileName};
        const pico::Uint64 creationTime = (pico::Uint64)(findData.ftCreationTime.dwHighDateTime) << 32u |
                                          findData.ftCreationTime.dwLowDateTime;

        if (nameView.ends_with(L".json") && creationTime > latestJSONBootLogCreateTime)
        {
            // Could only copy the string + null term, but this should be fine for a one time call
            std::copy(findData.cFileName, &findData.cFileName[MAX_PATH], latestJSONBootLogFileName.data());
            latestJSONBootLogCreateTime = creationTime;
        }
        else if (nameView.ends_with(L".log") && creationTime > latestRawBootLogCreateTime)
        {
            std::copy(findData.cFileName, &findData.cFileName[MAX_PATH], latestRawBootLogFileName.data());
            latestRawBootLogCreateTime = creationTime;
        }

    } while (FindNextFileW(findIterator.get(), &findData));

    if (!latestRawBootLogCreateTime || !latestJSONBootLogCreateTime)
    {
        // That's... not right
        return {};
    }

    MeasuredBootData data{};




    // TODO
    return data;
}
