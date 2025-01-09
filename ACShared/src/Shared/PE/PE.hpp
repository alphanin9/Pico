#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>

/**
 * \brief Wrappers for can1357's LinuxPE library.
 */
namespace pico::shared::PE
{
using Image = win::image_x64_t;

/**
 * \brief Validate that the pointer belongs to a PE image, return a pointer to an image structure.
 * 
 * \param aPtrInImage A pointer inside of an image. If it's not an actual image (RtlPcToFileHeader() returns nullptr),
 * treated as image base and validated accordingly.
 *
 * \return If the pointer belongs to a PE image, returns a pointer to its start as a PE structure. Returns nullptr on
 * failure.
 */
pico::shared::PE::Image* GetImagePtr(_In_ void* const aPtrInImage) noexcept;

/**
 * \brief Validates a PE image.
 * \param aImage A pointer to an image structure that is being validated.
 * \return Whether or not the image is a valid PE structure.
 */
pico::Bool IsImageValid(_In_ const pico::shared::PE::Image* const aImage) noexcept;

/**
 * \brief Gets bounds of the image.
 * 
 * Minimum is aImage + BaseOfCode, maximum is aImage + SizeOfImage.
 *
 * \param aImage A pointer to an image structure the data is queried from.
 * 
 * \return A [min, max] pair of the image's minimum and maximum bounds.
 */
std::pair<uintptr_t, uintptr_t> GetImageBounds(_In_ const pico::shared::PE::Image* const aImage) noexcept;

/**
 * \brief Gathers all the function entries from the image's exception information directory and returns their bounds as RVAs.
 * 
 * \param aImage A pointer to an image structure the data is queried from.
 * 
 * \return A vector of pairs of [functionStartRva, functionEndRva] entries. Returns empty vector on failure.
 */
pico::Vector<std::pair<pico::Uint32, pico::Uint32>> GetFunctionsOfImage(
    _In_ const pico::shared::PE::Image* const aImage) noexcept;

/**
 * \brief Get a function's bounds.
 * 
 * Bounds are decided based on image exception information.
 * 
 * \param aImage A pointer to an image structure the data is queried from.
 * \param aAddrInFunc A pointer to an address that belongs within a function.
 * 
 * \return A pair of [functionMin, functionMax] function bounds, or [0, 0] on failure.
 */
std::pair<uintptr_t, uintptr_t> GetFunctionBounds(_In_ const pico::shared::PE::Image* const aImage,
                                                  _In_ const void* const aAddrInFunc) noexcept;

/**
 * \brief Checks if the total size of an image's read-only sections is smaller than 16 kB.
 * 
 * \param aImage A pointer to an image structure the data is queried from.
 * 
 * \return Whether or not the image is small for an integrity check.
 */
pico::Bool IsIntegrityCheckableImageSizeSmall(_In_ const pico::shared::PE::Image* const aImage) noexcept;

/**
 * \brief Checks if an image has a large amount of space (over 4 kB) that is readable, writable and executable.
 * 
 * This may signify a packed image or an image that is easy to take advantage of for breaking integrity checks.
 * 
 * \param aImage A pointer to an image structure the data is queried from.
 * 
 * \return Whether or not the image has large amounts of RWX space.
 */
pico::Bool HasLargeRWXSections(_In_ const pico::shared::PE::Image* const aImage) noexcept;

/**
 * \brief Gets an image's program debug database path.
 * 
 * \param aImage A pointer to an image structure the data is queried from.
 * 
 * \return The path to the program debug database file. If the information is not present or the function fails, returns an empty string.
 */
pico::String GetImagePDBPath(_In_ const pico::shared::PE::Image* const aImage) noexcept;

/**
 * \brief Enumerates an image's relocations of type IMAGE_REL_BASED_DIR64 and returns their RVAs.
 * 
 * Other relocations are not enumerated due to difficulties in handling, but are used on Windows.
 * A more proper implementation would replicate the behavior of ntdll!LdrProcessRelocationBlockLongLong.
 * 
 * \param aImage A pointer to an image structure the data is queried from.
 * 
 * \return A vector containing every RVA of an IMAGE_REL_BASED_DIR64 typed relocation in the image. If the function fails, returns an empty vector.
 */
pico::Vector<pico::Uint32> GetRelocations(_In_ const pico::shared::PE::Image* const aImage) noexcept;
} // namespace pico::shared::PE