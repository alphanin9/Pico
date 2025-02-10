#pragma once
namespace pico::shared::Util
{
/**
 * \brief Base class to make inherited classes non-copyable
 */
struct NonCopyable
{
    NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

/**
 * \brief Base class to make inherited classes non-copyable and non-movable
 */
struct NonCopyableOrMovable : public NonCopyable
{
    NonCopyableOrMovable() = default;
    ~NonCopyableOrMovable() = default;

    NonCopyableOrMovable(NonCopyableOrMovable&&) = delete;
    NonCopyableOrMovable& operator=(NonCopyableOrMovable&&) = delete;
};
} // namespace pico::shared::Util