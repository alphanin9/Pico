#pragma once
#include <Shared/Common.hpp>
#include <Shared/MemoryUtils/AOBPattern.hpp>
#include <Shared/ProcessEnv/ProcessEnv.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>

/*
 * This is a slightly modified version of
 * https://github.com/alphanin9/SharedPunk/blob/main/src/include/Public/Shared/Util/Core.hpp
 * Which originated from
 * https://github.com/psiberx/cp2077-codeware/blob/main/lib/Core/Raw.hpp
 */
namespace pico::shared
{
class RawBase
{
public:
    constexpr RawBase() = default;

    inline static uintptr_t GetImageBase()
    {
        // ...get first entry in Ldr list?
        static const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        return base;
    }

    inline static uintptr_t GetImageBase(pico::Uint64 aModuleHash)
    {
        const auto moduleData = pico::shared::ProcessEnv::GetModuleByHash(aModuleHash);

        if (!moduleData)
        {
            return 0u;
        }

        return reinterpret_cast<uintptr_t>(moduleData->DllBase);
    }
};

template<pico::shared::HashedString ModuleHash, pico::shared::StaticString Pattern, typename T>
class RawFunc
{
};

template<pico::shared::HashedString ModuleHash, pico::shared::StaticString Pattern, typename R, typename... Args>
class RawFunc<ModuleHash, Pattern, R (*)(Args...)> : public RawBase
{
public:
    using Type = R (*)(Args...);
    using Callable = Type;

    static constexpr AOBPattern<ModuleHash, Pattern> Target{};

    constexpr RawFunc() = default;

    operator bool() const noexcept
    {
        return GetPtr() != nullptr;
    }

    [[nodiscard]] inline operator Callable() const
    {
        return GetPtr();
    }

    [[nodiscard]] inline Callable operator~() const
    {
        return GetOriginalPtr();
    }

    [[nodiscard]] inline Callable GetPtr() const noexcept
    {
        return reinterpret_cast<Callable>(GetAddress());
    }

    [[nodiscard]] inline Callable GetOriginalPtr() const noexcept
    {
        return reinterpret_cast<Callable>(GetOriginalAddress());
    }

    inline static uintptr_t GetAddress() noexcept
    {
        if (!original)
        {
            ResetAddress();
        }

        return address;
    }

    inline static uintptr_t GetOriginalAddress() noexcept
    {
        if (!original)
        {
            ResetAddress();
        }

        return original;
    }

    inline static void ResetAddress() noexcept
    {
        original = Target.Resolve();

        address = original;
    }

    inline static void SetAddress(uintptr_t aAddress) noexcept
    {
        if (!original)
        {
            ResetAddress();
        }

        address = aAddress;
    }

    inline static auto Invoke(Args... aArgs)
    {
        return reinterpret_cast<Callable>(GetAddress())(std::forward<Args>(aArgs)...);
    }

    inline static auto InvokeOriginal(Args... aArgs)
    {
        return reinterpret_cast<Callable>(GetOriginalAddress())(std::forward<Args>(aArgs)...);
    }

private:
    inline static uintptr_t address = 0;
    inline static uintptr_t original = 0;
};

template<pico::shared::HashedString ModuleHash, pico::shared::StaticString Pattern, typename C, typename R,
         typename... Args>
class RawFunc<ModuleHash, Pattern, R (C::*)(Args...)> : public RawFunc<ModuleHash, Pattern, R (*)(C*, Args...)>
{
public:
    using Base = RawFunc<ModuleHash, Pattern, R (*)(C*, Args...)>;
    using Base::Base;
};

template<uintptr_t A, typename T>
class RawVFunc
{
};

template<uintptr_t A, typename C, typename R, typename... Args>
class RawVFunc<A, R (*)(C*, Args...)> : public RawBase
{
public:
    using Type = R (*)(C*, Args...);
    using Callable = Type;

    static constexpr uintptr_t offset = A;

    constexpr RawVFunc() = default;

    auto operator()(C* aContext, Args... aArgs) const
    {
        auto vft = *reinterpret_cast<uintptr_t*>(aContext);
        auto callable = *reinterpret_cast<Callable*>(vft + offset);
        return callable(aContext, std::forward<Args>(aArgs)...);
    }
};

template<auto A, typename C, typename R, typename... Args>
class RawVFunc<A, R (C::*)(Args...)> : public RawVFunc<A, R (*)(C*, Args...)>
{
public:
    using Base = RawVFunc<A, R (*)(C*, Args...)>;
    using Base::Base;
};

template<pico::shared::HashedString ModuleHash, pico::shared::StaticString Pattern, typename T>
class RawPtr : public RawBase
{
public:
    using Type = std::conditional_t<std::is_void_v<std::remove_pointer_t<T>>, void*, std::remove_pointer_t<T>>;

    static constexpr AOBPattern<ModuleHash, Pattern> Target{};
    static constexpr bool Indirect = std::is_pointer_v<T> && !std::is_void_v<std::remove_pointer_t<T>>;

    constexpr RawPtr() = default;

    operator bool() const noexcept
    {
        return GetPtr() != nullptr;
    }

    [[nodiscard]] inline operator Type&() const
    {
        if constexpr (Indirect)
        {
            return **GetPtr();
        }
        else
        {
            return *GetPtr();
        }
    }

    [[nodiscard]] inline operator Type*() const
    {
        if constexpr (Indirect)
        {
            return *GetPtr();
        }
        else
        {
            return GetPtr();
        }
    }

    [[nodiscard]] inline Type* operator->() const
    {
        if constexpr (Indirect)
        {
            return *GetPtr();
        }
        else
        {
            return GetPtr();
        }
    }

    [[nodiscard]] inline T& operator[](size_t aIndex) const
    {
        return GetPtr()[aIndex];
    }

    RawPtr& operator=(T&& aRhs) const noexcept
    {
        *GetPtr() = aRhs;
        return *this;
    }

    [[nodiscard]] inline T* GetPtr() const noexcept
    {
        return reinterpret_cast<T*>(GetAddress());
    }

    [[nodiscard]] inline T& GetRef() const noexcept
    {
        return *reinterpret_cast<T*>(GetAddress());
    }

    inline static uintptr_t GetAddress() noexcept
    {
        static uintptr_t addr = Target.Resolve();

        return addr;
    }

    inline static T* Get()
    {
        return reinterpret_cast<T*>(GetAddress());
    }

    inline static T& Ref()
    {
        return *reinterpret_cast<T*>(GetAddress());
    }
};

template<uintptr_t A, typename T>
class OffsetPtr
{
public:
    using Type = std::conditional_t<std::is_void_v<std::remove_pointer_t<T>>, void*, std::remove_pointer_t<T>>;

    static constexpr uintptr_t offset = A;
    static constexpr bool indirect = std::is_pointer_v<T> && !std::is_void_v<std::remove_pointer_t<T>>;

    constexpr OffsetPtr(uintptr_t aBase)
        : address(aBase + offset)
    {
    }

    constexpr OffsetPtr(const void* aBase)
        : address(reinterpret_cast<uintptr_t>(aBase) + offset)
    {
    }

    [[nodiscard]] inline operator bool() const
    {
        if constexpr (std::is_same_v<Type, bool>)
        {
            return GetValuePtr() && *GetValuePtr();
        }
        else
        {
            return GetValuePtr();
        }
    }

    [[nodiscard]] inline operator Type&() const
    {
        return *GetValuePtr();
    }

    [[nodiscard]] inline operator Type*() const
    {
        return GetValuePtr();
    }

    [[nodiscard]] inline Type* operator->() const
    {
        return GetValuePtr();
    }

    OffsetPtr& operator=(T&& aRhs) const noexcept
    {
        *GetPtr() = aRhs;
        return *this;
    }

    const OffsetPtr& operator=(const T& aRhs) const noexcept
    {
        *GetPtr() = aRhs;
        return *this;
    }

    [[nodiscard]] inline T* GetPtr() const noexcept
    {
        return reinterpret_cast<T*>(GetAddress());
    }

    [[nodiscard]] inline Type* GetValuePtr() const noexcept
    {
        if constexpr (indirect)
        {
            return *GetPtr();
        }
        else
        {
            return GetPtr();
        }
    }

    [[nodiscard]] inline uintptr_t GetAddress() const noexcept
    {
        return address;
    }

    inline static Type* Ptr(const void* aBase)
    {
        return OffsetPtr(aBase).GetValuePtr();
    }

    inline static Type& Ref(const void* aBase)
    {
        return *OffsetPtr(aBase).GetValuePtr();
    }

    inline static uintptr_t Addr(const void* aBase)
    {
        return reinterpret_cast<uintptr_t>(OffsetPtr(aBase).GetValuePtr());
    }

    inline static void Set(const void* aBase, const Type& aValue)
    {
        *OffsetPtr(aBase).GetValuePtr() = aValue;
    }

    inline static void Set(const void* aBase, T& aValue)
    {
        *OffsetPtr(aBase).GetPtr() = aValue;
    }

    uintptr_t address;
};
} // namespace pico::shared