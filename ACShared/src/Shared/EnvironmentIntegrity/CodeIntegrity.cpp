#include <Shared/EnvironmentIntegrity/CodeIntegrity.hpp>

pico::shared::EnvironmentIntegrity::CodeIntegrityConfig pico::shared::EnvironmentIntegrity::GetCodeIntegrityConfig() noexcept
{
    Windows::SYSTEM_CODEINTEGRITY_INFORMATION info{};

    info.Length = sizeof(info);

    pico::Uint32 sizeWritten{};

    Windows::NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS::SystemCodeIntegrityInformation, &info,
                                      sizeof(info), sizeWritten);

    CodeIntegrityConfig config{};

    config.m_raw = info.CodeIntegrityOptions;
    
    config.m_codeIntegrity = (info.CodeIntegrityOptions & CODEINTEGRITY_OPTION_ENABLED) != 0;
    config.m_hypervisorCodeIntegrity = (info.CodeIntegrityOptions & CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED) != 0 &&
                                       (info.CodeIntegrityOptions & CODEINTEGRITY_OPTION_HVCI_KMCI_STRICTMODE_ENABLED) != 0;

    config.m_testSigning = (info.CodeIntegrityOptions & CODEINTEGRITY_OPTION_TESTSIGN) != 0;
    config.m_debugMode = (info.CodeIntegrityOptions & CODEINTEGRITY_OPTION_DEBUGMODE_ENABLED) != 0;
    
    return config;
}
