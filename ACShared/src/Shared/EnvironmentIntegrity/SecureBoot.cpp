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
    // TODO
    return {};
}