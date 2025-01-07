#include <Shared/Disassembler/Disassembler.hpp>

pico::Vector<INSTRUX> pico::shared::Disassembler::DisassembleRegion(uintptr_t aStartAddress,
                                                                    uintptr_t aEndAddress) noexcept
{
    if (!aStartAddress || (aEndAddress && (aStartAddress >= aEndAddress)))
    {
        return {};
    }

    pico::Int64 sizeOfCodeBuffer = aEndAddress - aStartAddress;

    INSTRUX instruction{};

    auto status =
        NdDecode(&instruction, reinterpret_cast<ND_UINT8*>(aStartAddress), ND_CODE_64, ND_DATA_64);

    sizeOfCodeBuffer -= instruction.Length;
    aStartAddress += instruction.Length;

    if (!ND_SUCCESS(status))
    {
        return {};
    }

    pico::Vector<INSTRUX> instructions{};

    // Note: we should be using recursive disassembler, this implementation is bad

    while (ND_SUCCESS(status) && sizeOfCodeBuffer > 0 && instruction.Category != ND_CAT_RET)
    {
        instructions.push_back(instruction);
        status = NdDecode(&instruction, reinterpret_cast<ND_UINT8*>(aStartAddress), ND_CODE_64,
                            ND_DATA_64);

        aStartAddress += instruction.Length;
        sizeOfCodeBuffer -= instruction.Length;
    }

    return instructions;
}