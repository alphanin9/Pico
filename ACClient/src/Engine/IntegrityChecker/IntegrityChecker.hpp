#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{

/**
 * \brief Container for PE file data
 */
struct ModuleData
{
    // Last integrity check time
    std::chrono::high_resolution_clock::time_point m_lastIntegrityCheckTime{};

    // Module path
    pico::UnicodeString m_path{};

    // Raw size of a module
    pico::Size m_rawSize{};

    // Processed data of a PE file
    pico::Vector<pico::Uint8> m_modulePeFileData{};

    // Pointer to start of from-disk PE
    shared::PE::Image* m_image{};

    // The size of the image's headers
    pico::Uint32 m_sizeOfHeaders{};

    // RVAs of image relocations, used for integrity checking
    pico::Vector<pico::Uint32> m_relocations{};

    // Image function entries from exception information
    pico::Vector<std::pair<pico::Uint32, pico::Uint32>> m_functionEntries{};

    // A SHA256 hash of the image
    pico::String m_sha256{};

    // Is the module signed and trusted?
    pico::Bool m_isTrusted{};

    /**
     * \brief Instantiate a module data entry based on loaded module data.
     *
     * \param aEntry The pointer to a loaded module data entry.
     * \param aBaseAddress The base address of the loaded module.
     * \return Whether or not loading failed.
     */
    pico::Bool Load(pico::UnicodeStringView aPath) noexcept;

    /**
     * \brief Applies relocations to the module data based on image relocation information.
     *
     * \param aBaseAddress The base address the integrity checked image was loaded at.
     * \return Whether or not relocating the image succeeded.
     */
    pico::Bool RelocateImage(void* aBaseAddress) noexcept;

    /**
     * \brief Dumps the module information to the logger. In production, this should be sent to the backend.
     */
    void DumpModuleInfo() noexcept;
};

/**
 * \brief Module integrity check wrapper.
 *
 * Scan all modules for patches to read-only sections.
 * Ignore relocations.
 *
 * If it's a jump to someplace in .text, check the jump's target (up to some steps),
 * see if it's to a legitimate module
 *
 * If it's a patch to a virtual function table (determine by there being a reloc pointing to executable section at that
 * point), check where the new address goes.
 *
 * Report about modules with big RWX sections where there's not much to check for.
 *
 * Our own module is checked more thoroughly, with no patches being allowed at all.
 */
struct IntegrityChecker : public shared::Util::NonCopyableOrMovable
{
    std::chrono::high_resolution_clock::time_point m_lastTimeLoadedDllsReported{};

    // Map of our process's loaded modules. Key is hash of module full path.
    pico::HashMap<pico::Uint64, ModuleData> m_moduleDataMap{};

    /**
     * \brief Scans a module for integrity based on exception information and image relocations.
     *
     * \param aModule The module representation from disk.
     * \param aImage The module representation in memory.
     * \param aIsClient Is the module our client?
     * \return Whether or not the module was tampered with.
     */
    pico::Bool ScanModule(pico::Engine::ModuleData& aModule, pico::shared::PE::Image* aImage,
                          pico::Bool aIsClient) const noexcept;

    /**
     * \brief Scans the client module for integrity.
     *
     * Note that this should NOT be the only place the client gets its integrity checked. A proper anti-tamper should be
     * checking its integrity from multiple places.
     *
     * \return Whether or not the client had integrity check failures.
     */
    pico::Bool ScanClient() noexcept;

    /**
     * \brief Ticks component in the thread pool worker.
     */
    void Tick() noexcept;

    /**
     * \brief Get a singleton instance of the integrity checker.
     * \return A singleton instance of the integrity checker.
     */
    static IntegrityChecker& Get() noexcept;
};
} // namespace pico::Engine
