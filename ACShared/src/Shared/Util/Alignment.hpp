#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>

namespace pico::shared::Util
{
/**
 * \brief Aligns a number down to aAlignment.
 * 
 * \tparam NumType The type of the number and alignment.
 * \param aNum The number.
 * \param aAlignment The desired alignment.
 * \return The number aligned down to aAlignment.
 */
template<typename NumType, typename = typename std::enable_if<std::is_integral<NumType>::value, NumType>::type>
inline constexpr NumType AlignDown(NumType aNum, NumType aAlignment)
{
    return aNum & ~(aAlignment - 1);
}

/**
 * \brief Aligns a number up to aAlignment.
 *
 * \tparam NumType The type of the number and alignment.
 * \param aNum The number.
 * \param aAlignment The desired alignment.
 * \return The number aligned up to aAlignment.
 */
template<typename NumType, typename = typename std::enable_if<std::is_integral<NumType>::value, NumType>::type>
inline constexpr NumType AlignUp(NumType aNum, NumType aAlignment)
{
    return AlignDown(aNum + (aAlignment - 1), aAlignment);
}
} // namespace pico::shared::Util
