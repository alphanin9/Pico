#include <Engine/DriverSnap/DriverSnap.hpp>
#include <Engine/Logging/Logger.hpp>

#include <Engine/DriverSnap/VulnerableDriverList.hpp>
#include <Engine/Engine.hpp>

pico::Engine::DriverSnap::EDriverResult pico::Engine::DriverSnap::ValidateDriver(
    const pico::shared::EnvironmentIntegrity::DriverInfo& aInfo, pico::Uint64& aFNV1aKey) const
{
    if (aInfo.m_fullPath.empty())
    {
        return EDriverResult::NoPath;
    }

    const auto sha256 = shared::Files::GetFileSHA256(aInfo.m_fullPath);

    if (sha256.empty())
    {
        return EDriverResult::NoSHA256;
    }

    aFNV1aKey = shared::FNV1a64(sha256.c_str());

    if (m_scannedDrivers.contains(aFNV1aKey))
    {
        return m_scannedDrivers.at(aFNV1aKey) ? EDriverResult::OK : EDriverResult::KnownBad;
    }

    if (Generated::VulnerableDriverHashes.contains(aFNV1aKey))
    {
        return EDriverResult::VulnerableByHash;
    }

    EngineThreadLoadGuard guard{};
    if (!shared::EnvironmentIntegrity::VerifyFileTrust(aInfo.m_fullPath,
                                                       shared::EnvironmentIntegrity::EFileType::Driver))
    {
        return EDriverResult::CertificateRevoked;
    }

    return EDriverResult::OK;
}

pico::Bool pico::Engine::DriverSnap::OnPreflight()
{
    auto isGood = true;

    auto& logger = Logger::GetLogSink();

    for (auto& driverInfo : shared::EnvironmentIntegrity::GetLoadedDriverPaths())
    {
        shared::Util::MsTaken profile{};
        pico::Uint64 key{};

        const auto status = ValidateDriver(driverInfo, key);

        switch (status)
        {
        case EDriverResult::NoPath:
            logger->warn("[Preflight - Drivers] Driver {} full path is empty! Cannot validate the driver without path.",
                         shared::Util::ToUTF8(driverInfo.m_rawPath));
            break;
        case EDriverResult::NoSHA256:
            logger->warn("[Preflight - Drivers] Failed to calculate driver {} SHA256 hash.",
                         shared::Util::ToUTF8(driverInfo.m_rawPath));
            break;
        case EDriverResult::VulnerableByHash:
            logger->error("[Preflight - Drivers] Driver {} is a known vulnerable driver!",
                          shared::Util::ToUTF8(driverInfo.m_rawPath));
            break;
        case EDriverResult::CertificateRevoked:
            logger->error("[Preflight - Drivers] Unable to validate driver {} signature!",
                          shared::Util::ToUTF8(driverInfo.m_rawPath));
            break;
        case EDriverResult::KnownBad:
            logger->error("[Preflight - Drivers] Bug: same bad driver {} loaded twice???",
                          shared::Util::ToUTF8(driverInfo.m_rawPath));
            break;
        default:
            break;
        }

        if (isGood)
        {
            isGood = status == EDriverResult::OK || status == EDriverResult::NoPath;
        }

        m_scannedDrivers.insert_or_assign(key, status == EDriverResult::OK || status == EDriverResult::NoPath);

        const auto taken = profile.Now();

        logger->info("[Preflight - Drivers] Time to verify {}: {}ms", shared::Util::ToUTF8(driverInfo.m_rawPath),
                     taken);
    }

    return isGood;
}

void pico::Engine::DriverSnap::Tick()
{
    const auto timestamp = pico::Clock::now();

    if ((timestamp - m_lastDriverCacheUpdateTime) > DriverCacheUpdateInterval)
    {
        m_driverCache = shared::EnvironmentIntegrity::GetLoadedDriverPaths();
        m_lastDriverCacheUpdateTime = timestamp;

        m_isDone = false;
        m_lastIndex = 0u;
    }

    if (m_isDone)
    {
        return;
    }

    const auto& entry = m_driverCache[m_lastIndex];

    pico::Uint64 key{};

    const auto status = ValidateDriver(entry, key);

    if (!m_scannedDrivers.contains(key))
    {
        m_scannedDrivers.insert_or_assign(key, status == EDriverResult::OK || status == EDriverResult::NoPath);
    }

    if (status != EDriverResult::OK && status != EDriverResult::NoPath && status != EDriverResult::KnownBad)
    {
        Logger::GetLogSink()->error("[Drivers] Driver {} seems to be bad! Status: {}",
                                    shared::Util::ToUTF8(entry.m_rawPath), pico::Int32(status));
    }

    m_lastIndex++;
    if (m_lastIndex == m_driverCache.size())
    {
        m_isDone = true;
    }
}

pico::Engine::DriverSnap& pico::Engine::DriverSnap::Get()
{
    static DriverSnap s_instance{};

    return s_instance;
}