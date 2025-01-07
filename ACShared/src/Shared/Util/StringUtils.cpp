#include <Shared/Util/StringUtils.hpp>

void pico::shared::ToUpperCase(_Inout_ pico::String& aStr) noexcept
{
    std::transform(aStr.cbegin(), aStr.cend(), aStr.begin(), [](pico::Char aChr) { return std::toupper(aChr); });
}

void pico::shared::ToLowerCase(_Inout_ pico::String& aStr) noexcept
{
    std::transform(aStr.cbegin(), aStr.cend(), aStr.begin(), [](pico::Char aChr) { return std::tolower(aChr); });
}