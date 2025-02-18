#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{
/**
 * \brief Checks loaded drivers for known bad actors. This can't be very efficient from user mode, as we have no
 * knowledge exactly when a driver gets loaded in. With driver assistance, Windows provides convenient callbacks to
 * validate a driver upon load (such as utilizing SeRegisterImageVerificationCallback, PsSetLoadImageNotifyRoutine or
 * various other callbacks).
 *
 * Since an attacker could theoretically swap a valid driver with a vulnerable one between checks, we can't store paths
 * of already validated drivers.
 */
struct DriverSnap : shared::Util::NonCopyableOrMovable
{
    static constexpr pico::Seconds DriverCacheUpdateInterval{30u};

    enum class EDriverResult
    {
        OK = 0,       // Nothing to whine about
        NoPath = 1,   // Path is screwed up for some reason and the file doesn't exist, happens in legit situations too
        NoSHA256 = 2, // Failed to get SHA256, odd...
        VulnerableByHash = 3,   // Driver is in precompiled vulnerable driver list
        CertificateRevoked = 4, // Driver certificate has been revoked and the driver is not considered trustworthy
        KnownBad = 5            // We've already checked this one's file, no need to run WinVerifyTrust again
    };

    // Already signature verified drivers
    // Key is FNV1a64 of SHA256 hash hexstring
    pico::HashMap<pico::Uint64, pico::Bool> m_scannedDrivers{};

    // TODO: figure out how to not go through the whole process every cache refresh
    pico::Vector<shared::EnvironmentIntegrity::DriverInfo> m_driverCache{};
    pico::Timestamp m_lastDriverCacheUpdateTime{};

    pico::Size m_lastIndex{};
    pico::Bool m_isDone{};

    /**
     * \brief Check whether or not a driver should be considered good.
     *
     * \param aDriverInfo A reference to the driver information struct given by environment integrity.
     * \param aFNV1aKey The FNV1a64 hash of the driver's SHA256 hash. Used as a hashmap key.
     * \return The status of the driver.
     */
    EDriverResult ValidateDriver(const shared::EnvironmentIntegrity::DriverInfo& aDriverInfo,
                                 pico::Uint64& aFNV1aKey) const;

    /**
     * \brief Handle preflight check for bad drivers.
     * \return Whether or not the preflight check passed.
     */
    pico::Bool OnPreflight();

    /**
     * \brief Ticks the component in the thread pool. Updates loaded drivers and, if not present in loaded driver list,
     * checks verification.
     */
    void Tick();

    /**
     * \brief Get an instance of the driver snapper.
     * \return An instance of the driver snapper.
     */
    static DriverSnap& Get();
};
} // namespace pico::Engine
