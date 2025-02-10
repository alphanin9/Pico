#include <Shared/Disassembler/Disassembler.hpp>
#include <Zydis/Zydis.h>

namespace Detail
{
/**
 * \brief A tiny wrapper over Zydis's decoding API.
 */
struct InstructionData
{
    uintptr_t m_addr{};

    ZydisDecodedInstruction m_instr{};
    ZydisDecodedOperand m_operands[10];

    /**
     * \brief Decodes an instruction and places its result in aRet.
     *
     * \param aAddr The address of the instruction.
     * \param aDecoder A pointer to the Zydis instruction decoder.
     * \param aRet Where the instruction data will be placed.
     * \return Whether or not decoding succeeded.
     */
    static pico::Bool Get(uintptr_t aAddr, ZydisDecoder& aDecoder, InstructionData& aRet)
    {
        // No, Zydis, instructions *can* be longer than 15 bytes - evidently
        const auto ret = (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&aDecoder, reinterpret_cast<void*>(aAddr), 64u,
                                                              &aRet.m_instr, aRet.m_operands)) == ZYAN_TRUE);

        if (ret)
        {
            aRet.m_addr = aAddr;
        }

        return ret;
    }

    /**
     * \brief Did we disassemble alright and are we constructed properly?
     * \return Whether or not the instruction data is valid.
     */
    pico::Bool IsValid() const
    {
        return m_addr != 0u;
    }

    InstructionData() = default;

    InstructionData(ZydisDecoder& aDecoder, uintptr_t aAddr)
    {
        Get(aAddr, aDecoder, *this);
    }
};

enum class EResolution
{
    JustJmp,   // jmp %something%;
    MovImmJmp, // mov %immreg64%, %something%; jmp %immreg64%;
    Other,     // Everything else
    Failure    // Failed disasm
};

EResolution GetNextForAddressStack(ZydisDecoder& aDecoder, uintptr_t aCurrent, uintptr_t& aNext,
                                   pico::Int32& aDecodedInstructions, pico::Size& aDecodeSize)
{
    // Note: this is really ugly code and heavy on our stack as well
    InstructionData firstDecoded(aDecoder, aCurrent);

    if (!firstDecoded.IsValid())
    {
        return EResolution::Failure;
    }

    aDecodedInstructions++;
    aDecodeSize += firstDecoded.m_instr.length;

    // First instruction is JMP, check if it's the right kind - if it's not, bail
    if (firstDecoded.m_instr.mnemonic == ZYDIS_MNEMONIC_JMP)
    {
        if (firstDecoded.m_operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
            firstDecoded.m_operands[0].imm.is_relative)
        {
            ZydisCalcAbsoluteAddress(&firstDecoded.m_instr, &firstDecoded.m_operands[0], firstDecoded.m_addr, &aNext);

            return EResolution::JustJmp;
        }
        else if (firstDecoded.m_operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                 firstDecoded.m_operands[0].mem.base == ZYDIS_REGISTER_RIP &&
                 firstDecoded.m_operands[0].mem.disp.has_displacement)
        {
            // We need to get displacement first
            // Yes, this is fun
            uintptr_t dispAddr{};

            ZydisCalcAbsoluteAddress(&firstDecoded.m_instr, &firstDecoded.m_operands[0], firstDecoded.m_addr,
                                     &dispAddr);

            aNext = *reinterpret_cast<uintptr_t*>(dispAddr);

            return EResolution::JustJmp;
        }
    }
    else if (firstDecoded.m_instr.mnemonic == ZYDIS_MNEMONIC_MOV)
    {
        if (firstDecoded.m_operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
            firstDecoded.m_operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE && firstDecoded.m_operands[1].size == 64u)
        {
            const auto immregNum = firstDecoded.m_operands[0].reg.value;
            const auto immregValue = firstDecoded.m_operands[1].imm.value.u;

            InstructionData nextDecoded(aDecoder, aCurrent + firstDecoded.m_instr.length);

            if (!nextDecoded.IsValid())
            {
                return EResolution::Failure;
            }

            aDecodedInstructions++;
            aDecodeSize += nextDecoded.m_instr.length;

            if (nextDecoded.m_instr.mnemonic == ZYDIS_MNEMONIC_JMP)
            {
                if (nextDecoded.m_operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                    nextDecoded.m_operands[0].reg.value == immregNum)
                {
                    aNext = immregValue;
                    return EResolution::MovImmJmp;
                }
            }
        }
    }

    return EResolution::Other;
}
} // namespace Detail

pico::shared::Disassembler::EJumpFollowState pico::shared::Disassembler::FollowJumpChain(uintptr_t aAddress,
                                                                                         uintptr_t& aResult,
                                                                                         pico::Size& aSizeOfInstruction)
{
    ZydisDecoder decoder{};

    // No need to handle errors here, it's fine...
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

    constexpr auto MaxInstructionCount = 16;

    // Pop off the back of this to get the next disassembled address.
    pico::Vector<uintptr_t> addressStack{};

    // To keep things sane
    auto decodedInstrCount = 0;

    uintptr_t firstAddr{};

    const auto firstStatus =
        Detail::GetNextForAddressStack(decoder, aAddress, firstAddr, decodedInstrCount, aSizeOfInstruction);

    if (firstStatus == Detail::EResolution::Other)
    {
        return EJumpFollowState::NotDetour;
    }
    else if (firstStatus == Detail::EResolution::Failure)
    {
        return EJumpFollowState::DisassemblyFailure;
    }

    addressStack.push_back(firstAddr);

    while (!addressStack.empty() && decodedInstrCount < MaxInstructionCount)
    {
        // The size of instruction param is only to help integrity check, we don't need it after the first decoded instr
        pico::Size dummySize{};

        // Get next address, pop the old one off
        auto curAddr = addressStack.back();
        addressStack.pop_back();

        uintptr_t nextAddr{};

        const auto status = Detail::GetNextForAddressStack(decoder, curAddr, nextAddr, decodedInstrCount, dummySize);

        if (status == Detail::EResolution::Failure)
        {
            return EJumpFollowState::DisassemblyFailure;
        }
        else if (status == Detail::EResolution::Other)
        {
            aResult = curAddr;
            return EJumpFollowState::Success;
        }

        if (nextAddr)
        {
            addressStack.push_back(nextAddr);
        }
    }

    if (decodedInstrCount >= MaxInstructionCount)
    {
        return EJumpFollowState::InstructionLimit;
    }

    return EJumpFollowState::Unknown;
}