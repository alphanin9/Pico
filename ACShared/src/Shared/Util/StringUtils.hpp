#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>

namespace pico::shared
{
// In-place transforms the string to uppercase
void ToUpperCase(_Inout_ pico::String& aStr);

// In-place transforms the string to lowercase
void ToLowerCase(_Inout_ pico::String& aStr);
} // namespace pico::shared