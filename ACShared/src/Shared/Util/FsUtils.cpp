#include <Shared/Util/FsUtils.hpp>
#include <Shared/Util/NtdllCalls.hpp>

#include <openssl/evp.h>

namespace Detail
{
struct EVPContextFree
{
    void operator()(void* ptr)
    {
        EVP_MD_CTX_free((EVP_MD_CTX*)ptr);
    }
};

template<typename T>
using EVPUniquePtr = std::unique_ptr<T, EVPContextFree>;

struct OpenSSLFree
{
    void operator()(void* aPtr)
    {
        CRYPTO_free(aPtr, nullptr, 0);
    }
};

template<typename T>
using OpenSSLUniquePtr = std::unique_ptr<T, OpenSSLFree>;
} // namespace Detail

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
    std::error_code ec{};

    const auto size = std::filesystem::file_size(aPath, ec);

    if (ec)
    {
        return {};
    }

    pico::Vector<pico::Uint8> fileContent(size, {});
    std::ifstream file(aPath.data(), std::ios::binary);

    if (!file.good())
    {
        return {};
    }

    file.read(reinterpret_cast<char*>(fileContent.data()), size);

    Detail::EVPUniquePtr<EVP_MD_CTX> evpContext{EVP_MD_CTX_new()};

    if (!evpContext)
    {
        return {};
    }

    if (!EVP_DigestInit_ex(evpContext.get(), EVP_sha256(), nullptr))
    {
        return {};
    }

    if (!EVP_DigestUpdate(evpContext.get(), fileContent.data(), fileContent.size()))
    {
        return {};
    }

    std::array<pico::Uint8, EVP_MAX_MD_SIZE> hashStackBuffer{};

    pico::Uint32 hashLength{};

    if (!EVP_DigestFinal_ex(evpContext.get(), hashStackBuffer.data(), &hashLength))
    {
        return {};
    }

    Detail::OpenSSLUniquePtr<pico::Char> sslStr{OPENSSL_buf2hexstr(hashStackBuffer.data(), hashLength)};

    // Explicitly copy the string
    pico::String str = sslStr.get();

    return str;
}