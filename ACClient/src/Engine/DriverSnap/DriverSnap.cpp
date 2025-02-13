#include <Engine/DriverSnap/DriverSnap.hpp>
#include <Engine/Logging/Logger.hpp>

#include <Engine/DriverSnap/VulnerableDriverList.hpp>

pico::Bool pico::Engine::DriverSnap::OnPreflight()
{
    auto isGood = true;

    auto& logger = Logger::GetLogSink();

    for (auto& driverInfo : shared::EnvironmentIntegrity::GetLoadedDriverPaths())
    {
        shared::Util::MsTaken profile{};

        if (driverInfo.m_fullPath.empty())
        {
            logger->warn("[Preflight - Drivers] Driver {} full path is empty! Cannot validate the driver without path.",
                         shared::Util::ToUTF8(driverInfo.m_rawPath));
            continue;
        }

        const auto sha256 = shared::Files::GetFileSHA256(driverInfo.m_fullPath);

        if (sha256.empty())
        {
            logger->warn("[Preflight - Drivers] Failed to calculate driver {} SHA256 hash.",
                         shared::Util::ToUTF8(driverInfo.m_rawPath));
            continue;
        }

        const auto shaHash = shared::FNV1a64(sha256.c_str());

        if (Generated::VulnerableDriverHashes.contains(shaHash))
        {
            logger->error("[Preflight - Drivers] Driver {} is a known vulnerable driver!",
                          shared::Util::ToUTF8(driverInfo.m_rawPath));
        }

        const auto sigResult = shared::EnvironmentIntegrity::VerifyFileTrust(
            driverInfo.m_fullPath, shared::EnvironmentIntegrity::EFileType::Driver);

        m_scannedDrivers.insert_or_assign(shaHash, sigResult);

        if (!sigResult)
        {
            isGood = false;
            logger->error("[Preflight - Drivers] Unable to validate driver {} signature!",
                          shared::Util::ToUTF8(driverInfo.m_rawPath));
        }

        const auto taken = profile.Now();

        logger->info("[Preflight - Drivers] Time to verify {}: {}ms", shared::Util::ToUTF8(driverInfo.m_rawPath),
                     taken);
    }

    return isGood;
}

pico::Engine::DriverSnap& pico::Engine::DriverSnap::Get()
{
    static DriverSnap s_instance{};

    return s_instance;
}