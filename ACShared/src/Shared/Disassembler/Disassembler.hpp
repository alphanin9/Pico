#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>

#include <bddisasm/bddisasm.h>

namespace pico::shared::Disassembler
{
pico::Vector<INSTRUX> DisassembleRegion(uintptr_t aStartAddress, uintptr_t aEndAddress = 0u) noexcept;
}