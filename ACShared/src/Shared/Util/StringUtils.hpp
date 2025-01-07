#pragma once
#include <Shared/StdlibTypeDefinitions.hpp>
#include <Shared/Common.hpp>

namespace pico::shared
{
// In-place transforms the string to uppercase
void ToUpperCase(_Inout_ pico::String& aStr) noexcept;

// In-place transforms the string to lowercase
void ToLowerCase(_Inout_ pico::String& aStr) noexcept;
}