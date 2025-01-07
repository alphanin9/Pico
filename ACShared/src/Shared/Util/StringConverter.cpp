#include <simdutf.h>

#include <Shared/Util/StringConverter.hpp>

pico::String pico::shared::Util::ToUTF8(pico::UnicodeStringView aView) noexcept
{
    // Evil cast
    const auto expectedSize = simdutf::utf8_length_from_utf16(reinterpret_cast<const char16_t*>(aView.data()), aView.size());

    pico::String output(expectedSize, '\0');

    // Maybe we should assert if expectedSize != received size?
    simdutf::convert_utf16_to_utf8(reinterpret_cast<const char16_t*>(aView.data()), aView.size(), output.data());

    return output;
}

pico::UnicodeString pico::shared::Util::ToUTF16(pico::StringView aView) noexcept
{
    const auto expectedSize = simdutf::utf16_length_from_utf8(aView.data(), aView.size());

    pico::UnicodeString output(expectedSize, L'\0');

    simdutf::convert_utf8_to_utf16(aView.data(), aView.size(), reinterpret_cast<char16_t*>(output.data()));

    return output;
}