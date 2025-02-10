#include <Shared/Util/StringConverter.hpp>

pico::String pico::shared::Util::ToUTF8(pico::UnicodeStringView aView)
{
    const auto bufferSize = WideCharToMultiByte(CP_UTF8, 0u, aView.data(), static_cast<pico::Int32>(aView.size()),
                                                nullptr, 0, nullptr, nullptr);

    pico::String output(pico::Size(bufferSize), '\0');

    WideCharToMultiByte(CP_UTF8, 0u, aView.data(), static_cast<pico::Int32>(aView.size()), output.data(), output.size(),
                        nullptr, nullptr);

    return output;
}

pico::UnicodeString pico::shared::Util::ToUTF16(pico::StringView aView)
{
    const auto bufferSize =
        MultiByteToWideChar(CP_UTF8, 0u, aView.data(), static_cast<pico::Int32>(aView.size()), nullptr, 0);

    pico::UnicodeString output(pico::Size(bufferSize), L'\0');

    MultiByteToWideChar(CP_UTF8, 0u, aView.data(), static_cast<pico::Int32>(aView.size()), output.data(),
                        output.size());

    return output;
}