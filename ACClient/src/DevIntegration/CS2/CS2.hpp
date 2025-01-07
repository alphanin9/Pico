#pragma once
#include <Shared/Pico.hpp>

namespace pico::Integration::CS2
{
void OnReflexDX11(void* aRcx);
void OnReflexVulkan(void* aRcx);

pico::Bool InitCS2() noexcept;
}