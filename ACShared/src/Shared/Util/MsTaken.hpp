#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>
#include <Shared/Util/NonCopyable.hpp>

namespace pico::shared::Util
{
/**
 * \brief A helper class to get a time measurement for various things.
 */
struct MsTaken : pico::shared::Util::NonCopyableOrMovable
{
    pico::Timestamp m_start{};

    MsTaken();

    pico::Uint64 Now() const;
};
} // namespace pico::shared::Util
