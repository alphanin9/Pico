#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>

/**
 * \brief A small API for the disassembler.
 */
namespace pico::shared::Disassembler
{
/**
 * \brief Describes the result of following relative jumps.
 */
enum class EJumpFollowState
{
    Success,            // Successfully followed to some place
    NotDetour,          // First instructions were not JMP %somewhere%; or MOV %reg%, %64-bit value%; JMP %reg%;
    InstructionLimit,   // Hit maximum instruction count
    DisassemblyFailure, // Failed to disassemble an instruction
    Unknown             // Shouldn't ever hit this, we somehow ended up exiting the address stack without dying someplace else
};

/**
 * \brief Follows a chain of absolute/relative jumps until either finding the destination,
 * hitting the maximum jump limit of 16 or hitting something we couldn't disassemble.
 *
 * \param aAddress The address of the first jump instruction.
 * \param aResult Where the final value will be placed, assuming the function succeeded.
 * \param aSizeOfInstruction If the first jump chain instructions disassembled fine, their combined size to skip them in
 * case of an integrity check.
 *
 * \return A status code describing the result of the function.
 */
EJumpFollowState FollowJumpChain(uintptr_t aAddress, uintptr_t& aResult, pico::Size& aSizeOfInstruction) noexcept;
} // namespace pico::shared::Disassembler