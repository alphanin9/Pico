#pragma once
#include <Shared/Pico.hpp>

namespace pico::Integration::Rust
{
void OnReflex(void* aRcx);

pico::Bool InitRust();
} // namespace pico::Integration::Rust