#include <Shared/SystemEnv/ProcessEnumerator.hpp>

pico::Bool pico::shared::SystemEnv::FillSystemProcessInformationBuffer(pico::Vector<pico::Uint8>& aBuffer)
{
    constexpr pico::Size MinimumBufferSize = 0x4000u;

    if (aBuffer.size() < MinimumBufferSize)
    {
        aBuffer.assign(MinimumBufferSize, {});
    }
    else
    {
        aBuffer.assign(aBuffer.size(), {});
    }

    pico::Uint32 sizeNeeded{};

    auto status =
        Windows::NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS::SystemExtendedProcessInformation,
                                          aBuffer.data(), (pico::Uint32)(aBuffer.size()), sizeNeeded);

    // Resize it until we don't get error anymore
    // aBuffer.assign(...) shouldn't be done here - we don't need it
    while (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        aBuffer.resize(sizeNeeded);
        status =
            Windows::NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS::SystemExtendedProcessInformation,
                                              aBuffer.data(), (pico::Uint32)(aBuffer.size()), sizeNeeded);
    }

    return NT_SUCCESS(status);
}
