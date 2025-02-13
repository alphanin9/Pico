#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{
/**
 * \brief Checks loaded drivers for known bad actors.
 */
struct DriverSnap : shared::Util::NonCopyableOrMovable
{
    // Already signature verified drivers
    // Key is FNV1a64 of SHA256 hash hexstring
    pico::HashMap<pico::Uint64, pico::Bool> m_scannedDrivers{};

    /**
     * \brief Handle preflight check for bad drivers.
     * \return Whether or not the preflight check passed.
     */
    pico::Bool OnPreflight();

    static DriverSnap& Get();
};
}
