#include <Engine/Engine.hpp>
#include <Engine/IntegrityChecker/IntegrityChecker.hpp>
#include <Engine/Logging/Logger.hpp>

pico::Bool pico::Engine::ModuleData::Load(pico::UnicodeStringView aModulePath) noexcept
{
    pico::Path filePath = aModulePath;

    std::error_code ec{};

    // This shouldn't happen
    if (!std::filesystem::exists(filePath, ec))
    {
        return false;
    }

    auto fileSize = std::filesystem::file_size(filePath, ec);

    // Again, if the file doesn't exist or we die while trying to read it,
    // how is it loaded?
    if (ec)
    {
        return false;
    }

    // Note: Use two vectors instead, one for the raw file content, one for the actual PE file
    pico::Vector<pico::Uint8> rawPeData(fileSize, {});

    std::ifstream file(filePath, std::ios::binary);

    // Read the entirety of the file from disk
    // Maybe MapViewOfFile is better?
    file.read(reinterpret_cast<char*>(&rawPeData[0]), fileSize);

    auto rawImage = shared::PE::GetImagePtr(rawPeData.data());

    if (!rawImage)
    {
        return false;
    }

    // Reset module data, fill with 0s
    m_modulePeFileData.assign(rawImage->get_nt_headers()->optional_header.size_image, {});

    m_sizeOfHeaders = rawImage->get_nt_headers()->optional_header.size_headers;

    // Copy PE headers in
    std::copy_n(rawPeData.begin(), m_sizeOfHeaders, m_modulePeFileData.begin());

    // Copy sections
    for (const auto& section : rawImage->get_nt_headers()->sections())
    {
        std::copy_n(&rawPeData[section.ptr_raw_data], section.size_raw_data,
                    &m_modulePeFileData[section.virtual_address]);
    }

    m_image = shared::PE::GetImagePtr(m_modulePeFileData.data());

    if (!m_image)
    {
        return false;
    }

    return RelocateImage(m_image);
}

pico::Bool pico::Engine::ModuleData::RelocateImage(void* aBaseAddress) noexcept
{
    m_relocationDelta =
        reinterpret_cast<pico::Uint64>(aBaseAddress) - m_image->get_nt_headers()->optional_header.image_base;

    if (m_relocationDelta == 0u)
    {
        // The image does not need relocations to be performed
        return true;
    }

    m_relocations = shared::PE::GetRelocations(m_image);

    for (auto relocRva : m_relocations)
    {
        *m_image->raw_to_ptr<pico::Uint64>(relocRva) += m_relocationDelta;
    }

    return true;
}

pico::Bool pico::Engine::IntegrityChecker::ScanModule(pico::Engine::ModuleData& aModule,
                                                      pico::shared::PE::Image* aImage,
                                                      pico::Bool aIsClientModule) const noexcept
{
    static auto& s_engine = Engine::Get();
    auto& logger = Logger::GetLogSink();

    // We do not need to do the same check for on-disk image, ModuleData::Load() already does it
    if (!aImage)
    {
        logger->error("Module in memory did not have valid PE!");
        return false;
    }

    // We should dump such images
    if (pico::shared::PE::HasLargeRWXSections(aModule.m_image))
    {
        logger->warn("Image has large RWX sections! This is potentially a sign of cheating.");
    }

    if (pico::shared::PE::IsIntegrityCheckableImageSizeSmall(aModule.m_image))
    {
        logger->warn("Image has not much read-only to check!");
    }

    // Check PE structure
    // I don't think this should differ too much at disk and runtime, assuming you fix everything up correctly
    // Note: something else besides image base also needs fixing, unsure what

    auto peHeadersEqual = true;

    const auto backupImageBase = aModule.m_image->get_nt_headers()->optional_header.image_base;
    aModule.m_image->get_nt_headers()->optional_header.image_base = reinterpret_cast<pico::Uint64>(aImage);

    for (auto i = 0u; i < aModule.m_sizeOfHeaders; i++)
    {
        auto diskByte = *aModule.m_image->raw_to_ptr<pico::Uint8>(i);
        auto memByte = *aImage->raw_to_ptr<pico::Uint8>(i);

        if (diskByte != memByte)
        {
            logger->error("[Headers] Bytes at RVA {:#x} ({} {}) differ! {:x} != {:x}", i,
                            aModule.m_image->raw_to_ptr<void>(i), aImage->raw_to_ptr<void>(i), diskByte, memByte);
            peHeadersEqual = false;
        }
    }

    aModule.m_image->get_nt_headers()->optional_header.image_base = backupImageBase;

    if (!peHeadersEqual)
    {
        logger->warn("PE headers differ between disk and image!");
    }

    auto success = true;
    auto functionEntries = shared::PE::GetFunctionsOfImage(aModule.m_image);

    if (functionEntries.empty())
    {
        logger->error("Binary has no exception information!");
        return false;
    }

    // This is not perfectly optional
    for (auto [startRva, endRva] : functionEntries)
    {
        for (auto i = startRva; i != endRva; i++)
        {
            auto diskByte = *aModule.m_image->raw_to_ptr<pico::Uint8>(i);
            auto memByte = *aImage->raw_to_ptr<pico::Uint8>(i);

            // Disable this in production!!!
            constexpr auto IgnoreBreakpoints = true;

            if (diskByte != memByte)
            {
                if (IgnoreBreakpoints && memByte == 0xCC)
                {
                    // Ignore the breakpoint, go on as usual...
                    // Note: child process debugger plugin will break on CreateProcess stuff
                    // I break on lots of things myself
                    continue;
                }

                // TODO: if it's a jump, follow jump chain
                logger->error("Bytes at RVA {:#x} ({} {}) differ! {:x} != {:x}", i,
                              aModule.m_image->raw_to_ptr<void>(i), aImage->raw_to_ptr<void>(i), diskByte, memByte);

                success = false;
            }
        }
    }

    // (Sorta) find vfunc pointer swaps by checking relocations
    // Note: for whatever reason, this points to lots of other things around WinAPI too - could they be doing VFT swaps
    // or is this an imports thing I haven't gotten the hang of yet?
    for (pico::Uint32 reloc : aModule.m_relocations)
    {
        auto relocSection = aModule.m_image->rva_to_section(reloc);

        // Skip relocs in writable sections
        if (!relocSection || relocSection->characteristics.mem_write == 1)
        {
            continue;
        }

        // Get the pointer
        auto diskRelocPtr = *aModule.m_image->raw_to_ptr<void*>(reloc);

        // Get its RVA
        auto diskRelocatedRva = aModule.m_image->ptr_to_raw(diskRelocPtr);

        // Is the RVA OK?
        if (diskRelocatedRva == win::img_npos)
        {
            continue;
        }

        // Get the section the RVA is pointing to
        auto section = aModule.m_image->rva_to_section(diskRelocatedRva);

        // Skip non-executable sections
        if (!section || section->characteristics.mem_execute == 0)
        {
            continue;
        }

        auto memoryReloc = aImage->raw_to_ptr<void*>(reloc);
        auto memoryRelocPtr = *memoryReloc;

        void* pe{};

        RtlPcToFileHeader(memoryRelocPtr, &pe);

        // Reloc is supposed to point in ourselves, but it points god-knows-where?
        if (!pe)
        {
            MEMORY_BASIC_INFORMATION mbi{};

            if (!VirtualQuery(memoryRelocPtr, &mbi, sizeof(mbi)))
            {
                continue;
            }

            if (!shared::MemoryEnv::IsProtectionExecutable(mbi.Protect) &&
                !shared::MemoryEnv::IsProtectionExecutable(mbi.AllocationProtect))
            {
                continue;
            }

            logger->info("Relocation {} (source {}, RVA {}) points outside of an image! This is unlikely to be right.",
                         memoryRelocPtr, reinterpret_cast<void*>(memoryReloc), reloc);
            continue;
        }

        if (pe != aImage)
        {
            pico::UnicodeString imageName{};
            wil::GetModuleFileNameW(reinterpret_cast<HMODULE>(pe), imageName);

            logger->info("Relocation {} (source {}, RVA {}) points to module {} instead of correct module!",
                         memoryRelocPtr, reinterpret_cast<void*>(memoryReloc), reloc, shared::Util::ToUTF8(imageName));
        }
    }

    return success;
}

pico::Bool pico::Engine::IntegrityChecker::ScanClient() noexcept
{
    constexpr std::chrono::seconds ClientLibraryRefreshInterval{20};

    static auto& s_engine = Engine::Get();
    auto& s_logger = Logger::GetLogSink();

    pico::UnicodeString modulePath{};
    if (FAILED(wil::GetModuleFileNameW(reinterpret_cast<HMODULE>(s_engine.m_moduleBase), modulePath)))
    {
        // WTF?
        return false;
    }

    auto timestamp = std::chrono::high_resolution_clock::now();

    static ModuleData s_clientData{};

    if (timestamp - s_clientData.m_lastIntegrityCheckTime > ClientLibraryRefreshInterval)
    {
        if (s_clientData.Load(modulePath))
        {
            s_clientData.m_lastIntegrityCheckTime = timestamp;
        }
        else
        {
            s_logger->error("Failed to refresh client module!");
        }
    }

    return ScanModule(s_clientData, shared::PE::GetImagePtr(s_engine.m_moduleBase), true);
}

void pico::Engine::IntegrityChecker::Tick() noexcept
{
    constexpr std::chrono::seconds LoadedModuleListReportInterval{60};
    constexpr std::chrono::seconds ModuleIntegrityCheckInterval{20};

    auto& s_logger = Logger::GetLogSink();
    static const auto& s_engine = Engine::Get();

    auto timestamp = std::chrono::high_resolution_clock::now();

    auto loadedModules = shared::ProcessEnv::GetLoadedModuleList();

    pico::Set<pico::Uint64> presentModules{};

    auto shouldReport = (timestamp - m_lastTimeLoadedDllsReported) > LoadedModuleListReportInterval;

    if (shouldReport)
    {
        m_lastTimeLoadedDllsReported = timestamp;
    }

    if (!ScanClient())
    {
        s_logger->error("Client failed integrity check!");
    }

    auto moduleScanned = false;

    for (auto i = 0u; i < loadedModules.size(); i++)
    {
        auto entry = loadedModules[i];
        if (shouldReport)
        {
            s_logger->info("Module {} loaded, base address {}", shared::Util::ToUTF8(entry->FullDllName.Buffer),
                           entry->DllBase);
        }

        const auto hash = shared::FNV1a64WideCharStringWithLowerCase(entry->FullDllName.Buffer);

        presentModules.insert(hash);

        if (!m_moduleDataMap.contains(hash))
        {
            // Create entry for uncontained present module
            m_moduleDataMap.insert_or_assign(hash, ModuleData{});
        }

        if (entry->DllBase == s_engine.m_moduleBase)
        {
            // It's our module...
            // We integrity check it elsewhere
            continue;
        }

        if (moduleScanned)
        {
            continue;
        }

        auto& moduleEntry = m_moduleDataMap.at(hash);

        if (timestamp - moduleEntry.m_lastIntegrityCheckTime < ModuleIntegrityCheckInterval)
        {
            continue;
        }

        if (!moduleEntry.Load(entry->FullDllName.Buffer))
        {
            s_logger->error("Failed to load module entry for {}, base address {}!",
                            shared::Util::ToUTF8(entry->BaseDllName.Buffer), entry->DllBase);
            continue;
        }

        // We should not have too many consecutive integrity checks
        moduleEntry.m_lastIntegrityCheckTime = timestamp + std::chrono::seconds(i);

        s_logger->info("Scanning module {} at base {}", shared::Util::ToUTF8(entry->BaseDllName.Buffer),
                       entry->DllBase);

        const auto status = ScanModule(moduleEntry, shared::PE::GetImagePtr(entry->DllBase), false);

        if (status)
        {
            s_logger->info("Module {} at base {} passed its integrity check!",
                           shared::Util::ToUTF8(entry->BaseDllName.Buffer), entry->DllBase);
        }
        else
        {
            s_logger->error("Module {} at base {} failed its integrity check!",
                            shared::Util::ToUTF8(entry->BaseDllName.Buffer), entry->DllBase);
        }

        // Only two integrity checks should ever be done on each worker call - one for an arbitrary module,
        // another for the client
        moduleScanned = true;
    }

    pico::Set<pico::Uint64> unloadedModules{};

    for (const auto& [key, _] : m_moduleDataMap)
    {
        if (!presentModules.contains(key))
        {
            unloadedModules.insert(key);
        }
    }

    // Filter unloaded modules away
    for (auto i : unloadedModules)
    {
        m_moduleDataMap.erase(i);
    }
}

pico::Engine::IntegrityChecker& pico::Engine::IntegrityChecker::Get() noexcept
{
    static IntegrityChecker s_instance{};

    return s_instance;
}