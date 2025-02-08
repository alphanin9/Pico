#include <Shared/Util/FsUtils.hpp>
#include <Shared/Util/NtdllCalls.hpp>

#include <picosha2.h>

pico::UnicodeString pico::shared::Files::GetFullDriverFilePath(pico::UnicodeStringView aPath) noexcept
{
    Windows::UNICODE_STRING str{};

    Windows::RtlInitUnicodeString(str, aPath.data());

    Windows::OBJECT_ATTRIBUTES objectAttributes{};
    Windows::IO_STATUS_BLOCK statusBlock{};

    InitializeObjectAttributes(&objectAttributes, &str, OBJ_CASE_INSENSITIVE, nullptr, nullptr);

    wil::unique_handle fileRef{};

    const auto status = Windows::NtCreateFile(*fileRef.addressof(), GENERIC_READ | SYNCHRONIZE, &objectAttributes,
                                              &statusBlock, nullptr, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN,
                                              FILE_SYNCHRONOUS_IO_NONALERT | FILE_COMPLETE_IF_OPLOCKED, nullptr, 0u);

    if (!NT_SUCCESS(status))
    {
        return {};
    }

    pico::UnicodeString pathBuffer{};

    wil::GetFinalPathNameByHandleW(fileRef.get(), pathBuffer);

    return pathBuffer;
}

pico::String pico::shared::Files::GetFileSHA256(pico::UnicodeStringView aPath) noexcept
{
    auto [fileHandle, error] = wil::try_open_file(aPath.data());

    if (error)
    {
        return {};
    }

    LARGE_INTEGER fileSize{};

    if (!GetFileSizeEx(fileHandle.get(), &fileSize))
    {
        return {};
    }

    wil::unique_handle fileMapping{CreateFileMappingA(fileHandle.get(), nullptr, PAGE_READONLY, 0u, 0u, nullptr)};

    if (!fileMapping)
    {
        return {};
    }

    wil::unique_mapview_ptr mapView{MapViewOfFile(fileMapping.get(), FILE_MAP_READ, 0u, 0u, 0u)};

    if (!mapView)
    {
        return {};
    }

    auto begin = reinterpret_cast<pico::Uint8*>(mapView.get());
    auto end = begin + fileSize.QuadPart;

    std::array<pico::Uint8, picosha2::k_digest_size> buffer{};

    picosha2::hash256(begin, end, buffer);

    pico::String out{};
    out.reserve(picosha2::k_digest_size * 2u);

    constexpr pico::Char HexAlphabet[] = "0123456789ABCDEF";

    for (const auto byte : buffer)
    {
        out.push_back(HexAlphabet[(byte >> 4) & 0xF]);
        out.push_back(HexAlphabet[byte & 0xF]);
    }

    return out;
}