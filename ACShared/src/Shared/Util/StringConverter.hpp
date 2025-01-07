#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>

namespace pico
{
namespace shared
{
namespace Util
{
pico::String ToUTF8(pico::UnicodeStringView aView) noexcept;
pico::UnicodeString ToUTF16(pico::StringView aView) noexcept;
}
}
}