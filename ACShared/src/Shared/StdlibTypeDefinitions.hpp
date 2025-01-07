#pragma once

#include <Shared/Common.hpp>

/*
 * Type mapping to our namespace, because I don't like typing TiltedPhoques out every time
 * Ideally, the project would be using non-STL types with support for no exceptions and no RTTI
 * That could also be extended to provide allocator-independent containers that could then be used
 * in non-standard conditions (kernel mode? UEFI? Why not?)
 * However, that is the scope of a thesis of its own, and this one is already fairly big
 *
 * The TiltedPhoques types are no longer necessary, as STL types will use mimalloc anyway with
 * operator new override
 *
 * ===================================================================================================
 */

namespace pico
{
#pragma region STL
template<typename T>
using Vector = TiltedPhoques::Vector<T>;

template<typename T>
using Set = TiltedPhoques::Set<T>;

template<typename K, typename V>
using HashMap = TiltedPhoques::Map<K, V>;

using String = TiltedPhoques::String;
using UnicodeString = TiltedPhoques::WString;

using StringView = std::string_view;
using UnicodeStringView = std::wstring_view;

using Path = std::filesystem::path;

template<typename T>
using SharedPtr = TiltedPhoques::SharedPtr<T>;

template<typename T>
using UniquePtr = TiltedPhoques::UniquePtr<T>;

template<typename T, typename... Args>
inline pico::SharedPtr<T> MakeShared(Args&&... aArgs)
{
    return TiltedPhoques::MakeShared<T>(std::forward<Args>(aArgs)...);
}

template<typename T, typename... Args>
inline auto MakeUnique(Args&&... aArgs)
{
    return TiltedPhoques::MakeUnique<T>(std::forward<Args>(aArgs)...);
}
#pragma endregion

#pragma region Primitive types
using Uint8 = std::uint8_t;
using Uint16 = std::uint16_t;
using Uint32 = std::uint32_t;
using Uint64 = std::uint64_t;

using Int8 = std::int8_t;
using Int16 = std::int16_t;
using Int32 = std::int32_t;
using Int64 = std::int64_t;

using Bool = bool;
using Float = float;
using Double = double;
using Char = char;
using WChar = wchar_t;
using Size = size_t;
#pragma endregion
} // namespace pico

#pragma region WIL details
namespace wil
{
namespace details
{
// Because pico::UnicodeString is not actually wstring, we need a custom string_maker
template<>
struct string_maker<pico::UnicodeString>
{
    HRESULT make(_In_reads_opt_(length) PCWSTR source, size_t length) WI_NOEXCEPT
    try
    {
        m_value = source ? pico::UnicodeString(source, length) : pico::UnicodeString(length, L'\0');
        return S_OK;
    }
    catch (...)
    {
        return E_OUTOFMEMORY;
    }

    wchar_t* buffer()
    {
        return &m_value[0];
    }

    HRESULT trim_at_existing_null(size_t length)
    {
        m_value.erase(length);
        return S_OK;
    }

    pico::UnicodeString release()
    {
        return pico::UnicodeString(std::move(m_value));
    }

    static PCWSTR get(const pico::UnicodeString& value)
    {
        return value.c_str();
    }

private:
    pico::UnicodeString m_value;
};
} // namespace details
#pragma endregion
} // namespace wil

#pragma region Operator new override
/**
 * Override STL allocator for everything
 */

namespace pico
{
inline void* Alloc(const pico::Size aSize)
{
    return TiltedPhoques::Allocator::GetDefault()->Allocate(aSize);
}

inline void Free(void* aBlock)
{
    TiltedPhoques::Allocator::GetDefault()->Free(aBlock);
}
} // namespace pico

inline void* operator new(size_t size)
{
    return pico::Alloc(size);
}

inline void operator delete(void* p) noexcept
{
    pico::Free(p);
}

inline void* operator new[](size_t size)
{
    return pico::Alloc(size);
}

inline void operator delete[](void* p) noexcept
{
    pico::Free(p);
}

inline void* operator new(size_t size, const std::nothrow_t&) noexcept
{
    return pico::Alloc(size);
}

inline void* operator new[](size_t size, const std::nothrow_t&) noexcept
{
    return pico::Alloc(size);
}

inline void operator delete(void* p, const std::nothrow_t&) noexcept
{
    pico::Free(p);
}

inline void operator delete[](void* p, const std::nothrow_t&) noexcept
{
    pico::Free(p);
}

inline void operator delete(void* p, size_t) noexcept
{
    pico::Free(p);
}

inline void operator delete[](void* p, size_t) noexcept
{
    pico::Free(p);
}
#pragma endregion