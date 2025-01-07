#include <Shared/Util/FsUtils.hpp>
#include <Shared/Util/NtdllCalls.hpp>

pico::UnicodeString pico::shared::Files::GetFullDriverFilePath(pico::UnicodeStringView aPath) noexcept
{
    Windows::UNICODE_STRING str{};

    Windows::RtlInitUnicodeString(str, aPath.data());

    Windows::OBJECT_ATTRIBUTES objectAttributes{};
    Windows::IO_STATUS_BLOCK statusBlock{};

    InitializeObjectAttributes(&objectAttributes, &str, OBJ_CASE_INSENSITIVE, nullptr, nullptr);

    wil::unique_handle fileRef{};

    const auto status = Windows::NtCreateFile(*fileRef.addressof(), GENERIC_READ | SYNCHRONIZE, &objectAttributes, &statusBlock, nullptr, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_COMPLETE_IF_OPLOCKED, nullptr, 0u);

    if (!NT_SUCCESS(status)) {
        return {};
    }

    pico::UnicodeString pathBuffer{};

    wil::GetFinalPathNameByHandleW(fileRef.get(), pathBuffer);

    return pathBuffer;
}
