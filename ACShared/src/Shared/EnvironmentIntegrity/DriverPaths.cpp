#include <Shared/EnvironmentIntegrity/DriverPaths.hpp>
#include <Shared/Util/FsUtils.hpp>

pico::Vector<pico::shared::EnvironmentIntegrity::DriverInfo> pico::shared::EnvironmentIntegrity::
    GetLoadedDriverPaths() noexcept
{
    /* The more documented approach of using EnumDeviceDrivers + GetDeviceDriverFileNameW no longer works
     * due to Win11 24H2 requiring debug privilege to get driver image base and GetDeviceDriverFileNameW 
     * needing device driver image base
    */
 
    constexpr auto BufferSize = 0x1000;

    pico::Vector<pico::Uint8> buffer(BufferSize, {});

    pico::Uint32 sizeNeeded{};

    auto status = Windows::NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS::SystemModuleInformation,
                                                    buffer.data(), buffer.size(), sizeNeeded);

    if (!NT_SUCCESS(status) && status != STATUS_INFO_LENGTH_MISMATCH)
    {
        return {};   
    }

    while (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        buffer.assign(sizeNeeded, {});
        status = Windows::NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS::SystemModuleInformation,
                                                   buffer.data(), buffer.size(), sizeNeeded);
    }

    if (!NT_SUCCESS(status))
    {
        return {};
    }

    const auto moduleInfo = reinterpret_cast<Windows::RTL_PROCESS_MODULES*>(buffer.data());

    pico::Vector<DriverInfo> drivers{};

    drivers.reserve(moduleInfo->NumberOfModules);

    for (auto i = 0u; i < moduleInfo->NumberOfModules; i++)
    {
        auto& mod = moduleInfo->Modules[i];

        DriverInfo info{};

        info.m_rawPath = Util::ToUTF16(reinterpret_cast<pico::Char*>(mod.FullPathName));
        info.m_fullPath = Files::GetFullDriverFilePath(info.m_rawPath);

        drivers.push_back(std::move(info));
    }

    return drivers;
}
