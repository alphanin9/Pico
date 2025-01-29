#include <Engine/Logging/Logger.hpp>
#include <Engine/Specific/CS2/CS2.hpp>
#include <Zydis/Zydis.h>

namespace Detail
{
/**
 * \brief Reverse engineered structure depicting a Source 2 interface's data.
 */
struct InterfaceData
{
    using OnQuery = uintptr_t* (*)();
    /* 00, the function called to get the interface's address, generally looks something like lea rax, [rip+rel]; ret; */
    OnQuery m_queryFunc{};
    const pico::Char* m_interfaceName{}; // 08, the name of the interface
    InterfaceData* m_nextInterface{};    // 10, a pointer to the next interface of the module
};

/**
 * \brief Finds the interface list's base address using Zydis to disassemble the CreateInterface function.
 *
 * The general logic is disassembling the function until either hitting an instruction limit of 8 or finding an
 * instruction that loads a pointer into a register. If we find the necessary pointer,
 * we calculate its proper address and return it.
 *
 * \param aModule The module's base address.
 * \return The first InterfaceData structure of the module on success. Null pointer on failure.
 */
InterfaceData* FindStartOfInterfaceData(HMODULE aModule)
{
    constexpr auto MaxInstructionCount = 8;

    auto disassemblyCursor = reinterpret_cast<uintptr_t>(GetProcAddress(aModule, "CreateInterface"));

    if (!disassemblyCursor)
    {
        return nullptr;
    }

    ZydisDecoder decoder{};
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

    ZydisDecodedInstruction instr{};
    ZydisDecodedOperand operands[10]{};

    for (auto i = 0; i < MaxInstructionCount; i++)
    {
        if (ZYAN_FAILED(ZydisDecoderDecodeFull(&decoder, reinterpret_cast<void*>(disassemblyCursor),
                                               ZYDIS_MAX_INSTRUCTION_LENGTH, &instr, operands)))
        {
            // Wat?
            return nullptr;
        }

        if (instr.mnemonic == ZYDIS_MNEMONIC_MOV && operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
            operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY && operands[1].mem.disp.has_displacement)
        {
            uintptr_t absoluteAddr{};

            if (ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&instr, &operands[1], disassemblyCursor, &absoluteAddr)) &&
                absoluteAddr)
            {
                // A bit hacky, we should be checking if it leads to a valid place in the module as well
                return *reinterpret_cast<InterfaceData**>(absoluteAddr);
            }
        }

        // Increment the cursor
        disassemblyCursor += instr.length;
    }

    return nullptr;
}

using GetRegisteredModules = HMODULE* (*)();
} // namespace Detail

void pico::Engine::Specific::CS2::WalkInterfaces() noexcept
{
    auto& logger = Logger::GetLogSink();

    // Note: at the moment there's no integrity check performed for this function - should fix that
    // We do check export addresses with the relocation check (nifty thing!), so we should be safe-ish from the EAT
    // being replaced, I think
    static auto s_tier0 = shared::ProcessEnv::GetModuleByName("tier0.dll")->DllBase;
    static auto s_getRegisteredModules = reinterpret_cast<Detail::GetRegisteredModules>(
        GetProcAddress(reinterpret_cast<HMODULE>(s_tier0), "Plat_GetRegisteredModules"));

    auto moduleList = s_getRegisteredModules();

    // The registered module list is limited to 100 internally
    // NOTE: consider caching a lot of these things
    for (auto i = 0u; i <= 100u; i++)
    {
        auto moduleEntry = moduleList[i];

        if (!moduleEntry)
        {
            continue;
        }

        if (auto interfaceEntry = Detail::FindStartOfInterfaceData(moduleEntry))
        {
            const auto pe = reinterpret_cast<shared::PE::Image*>(moduleEntry);

            const auto imageMin = reinterpret_cast<uintptr_t>(moduleEntry);
            const auto imageMax = imageMin + pe->get_nt_headers()->optional_header.size_image;

            for (auto entry = interfaceEntry; entry != nullptr; entry = entry->m_nextInterface)
            {
                // NOTE: we should check if the interface data pointer leads to a pointer in read-write section of the module as well
                const auto interfaceVtable = *entry->m_queryFunc();

                // Interface vtable is out of bounds? That's certainly not what legitimate software would do.
                if (interfaceVtable < imageMin || interfaceVtable > imageMax)
                {
                    logger->error("[CS2] Vtable of interface {} at {} appears to be hooked!", entry->m_interfaceName,
                                  interfaceVtable);
                }
            }
        }
    }
}

void pico::Engine::Specific::CS2::Tick() noexcept
{
    WalkInterfaces();
}

pico::Engine::Specific::CS2& pico::Engine::Specific::CS2::Get() noexcept
{
    static CS2 s_instance{};

    return s_instance;
}
