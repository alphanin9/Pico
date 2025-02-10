#include <Engine/Engine.hpp>
#include <Engine/IntegrityChecker/IntegrityChecker.hpp>
#include <Engine/Logging/Logger.hpp>

pico::Bool pico::Engine::ModuleData::Load(pico::UnicodeStringView aModulePath)
{
    m_path = aModulePath;
    m_sha256 = shared::Files::GetFileSHA256(aModulePath);
    if (m_sha256.empty())
    {
        // This shouldn't happen...
        return false;
    }
    m_isTrusted = shared::EnvironmentIntegrity::VerifyFileTrust(aModulePath);

    pico::Path filePath = aModulePath;

    std::error_code ec{};

    // This shouldn't happen
    if (!std::filesystem::exists(filePath, ec))
    {
        return false;
    }

    if (ec)
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
    // Note: we read it two times due to SHA256 calc, should probably fix it - or create a file mapping instead
    pico::Vector<pico::Uint8> rawPeData(fileSize, {});

    std::ifstream file(filePath, std::ios::binary);

    // Read the entirety of the file from disk
    // Maybe MapViewOfFile is better? No, not sure if it lets us relocate right (though it should, as Ntdll uses it
    // internally)
    file.read(reinterpret_cast<char*>(&rawPeData[0]), fileSize);

    auto rawImage = shared::PE::GetImagePtr(rawPeData.data());

    if (!rawImage)
    {
        return false;
    }

    m_rawSize = rawPeData.size();

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

    m_functionEntries = shared::PE::GetFunctionsOfImage(m_image);

    return RelocateImage(m_image);
}

pico::Bool pico::Engine::ModuleData::RelocateImage(void* aBaseAddress)
{
    const auto relocationDelta =
        reinterpret_cast<pico::Uint64>(aBaseAddress) - m_image->get_nt_headers()->optional_header.image_base;

    if (relocationDelta == 0u)
    {
        // The image does not need relocations to be performed
        return true;
    }

    m_relocations = shared::PE::GetRelocations(m_image);

    for (auto relocRva : m_relocations)
    {
        *m_image->raw_to_ptr<pico::Uint64>(relocRva) += relocationDelta;
    }

    return true;
}

void pico::Engine::ModuleData::DumpModuleInfo()
{
    auto& logger = Logger::GetLogSink();

    logger->info("[IntegrityChecker] Module data dump...");
    logger->info("[IntegrityChecker] Module path: {}", shared::Util::ToUTF8(m_path));
    logger->info("[IntegrityChecker] Size of module: {}", m_rawSize);
    logger->info("[IntegrityChecker] Size of mapped module: {}", m_modulePeFileData.size());
    logger->info("[IntegrityChecker] Size of PE headers: {}", m_sizeOfHeaders);
    logger->info("[IntegrityChecker] Relocation count: {}", m_relocations.size());
    logger->info("[IntegrityChecker] Function entry count: {}", m_functionEntries.size());
    logger->info("[IntegrityChecker] Has large RWX sections: {}, is small for integrity: {}",
                 shared::PE::HasLargeRWXSections(m_image), shared::PE::IsIntegrityCheckableImageSizeSmall(m_image));

    logger->info("[IntegrityChecker] Raw SHA256: {}", m_sha256);
    logger->info("[IntegrityChecker] Is trusted by WinVerifyTrust: {}", m_isTrusted);
    logger->info("[IntegrityChecker] End of module data dump...");
}

pico::Bool pico::Engine::IntegrityChecker::ScanModule(pico::Engine::ModuleData& aModule,
                                                      pico::shared::PE::Image* aImage, pico::Bool aIsClientModule) const
{
    static auto& s_engine = Engine::Get();
    auto& logger = Logger::GetLogSink();

    // We do not need to do the same check for on-disk image, ModuleData::Load() already does it
    if (!aImage)
    {
        logger->error("[IntegrityChecker] Module in memory did not have valid PE!");
        return false;
    }

    // We should dump such images
    if (shared::PE::HasLargeRWXSections(aModule.m_image))
    {
        logger->warn("[IntegrityChecker] Image has large RWX sections! This is potentially a sign of cheating.");
    }

    if (shared::PE::IsIntegrityCheckableImageSizeSmall(aModule.m_image))
    {
        logger->warn("[IntegrityChecker] Image has not much read-only to check!");
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
            logger->error("[IntegrityChecker] [Headers] Bytes at RVA {:#x} ({} {}) differ! {:x} != {:x}", i,
                          aModule.m_image->raw_to_ptr<void>(i), aImage->raw_to_ptr<void>(i), diskByte, memByte);
            peHeadersEqual = false;
        }
    }

    aModule.m_image->get_nt_headers()->optional_header.image_base = backupImageBase;

    if (!peHeadersEqual)
    {
        logger->warn("[IntegrityChecker] PE headers differ between disk and image!");
    }

    auto success = true;

    if (aModule.m_functionEntries.empty())
    {
        // TODO: fallback to plain iterating RO sections
        logger->error("[IntegrityChecker] Binary has no exception information! Should fall back here");
        return false;
    }

    auto funcCoverage = 0u;
    auto partialCoverage = 0u;

    // This is not perfectly optimal
    for (auto [startRva, endRva] : aModule.m_functionEntries)
    {
        funcCoverage++;

        // Tiny optimization, most functions will be scanned with surface scan, still OK for catching detours at start -
        // which is what matters for most stuff
        constexpr auto MinimalScannedAmount = 16u;
        constexpr auto RandomConstant = 0x5u;

        auto endpoint = endRva;

        // Do NOT apply this to the client module ever!
        // Around 1 in 5 times funcs will be covered fully
        if ((__rdtsc() % RandomConstant) != 0u && !aIsClientModule)
        {
            partialCoverage++;
            endpoint = std::min(endRva, startRva + MinimalScannedAmount);
        }

        for (pico::Size i = startRva; i <= endpoint; i++)
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

                if (aIsClientModule)
                {
                    // Immediately react, no point tracing
                    return false;
                }

                pico::Size instrSize{};

                uintptr_t addyToDetourTarget{};

                const auto status = shared::Disassembler::FollowJumpChain(
                    reinterpret_cast<uintptr_t>(aImage->raw_to_ptr<pico::Uint8>(i)), addyToDetourTarget, instrSize);

                if (status == shared::Disassembler::EJumpFollowState::Success && addyToDetourTarget)
                {
                    if (s_engine.IsAddressInUs(addyToDetourTarget))
                    {
                        logger->info("[IntegrityChecker] Found our detour at {}!",
                                     reinterpret_cast<void*>(addyToDetourTarget));
                    }

                    const auto pe = shared::PE::GetImagePtr(reinterpret_cast<void*>(addyToDetourTarget));

                    if (!pe)
                    {
                        logger->error(
                            "[IntegrityChecker] Found detour at RVA {:#x} pointing to address {}! No backing image.", i,
                            reinterpret_cast<void*>(addyToDetourTarget));
                    }
                    else
                    {
                        pico::UnicodeString imageName{};
                        wil::GetModuleFileNameW(reinterpret_cast<HMODULE>(pe), imageName);

                        logger->warn("[IntegrityChecker] Found detour for RVA {:#x} at module {}, addr {}, RVA {:#x}",
                                     i, shared::Util::ToUTF8(imageName), reinterpret_cast<void*>(addyToDetourTarget),
                                     addyToDetourTarget - reinterpret_cast<uintptr_t>(pe));
                    }

                    i += instrSize;
                    continue;
                }

                if (status != shared::Disassembler::EJumpFollowState::DisassemblyFailure)
                {
                    // Skip over a bunch of bytes we disassembled
                    i += instrSize;
                }

                logger->error("[IntegrityChecker] Bytes at RVA {:#x} ({} {}) differ! {:x} != {:x}", i,
                              aModule.m_image->raw_to_ptr<void>(i), aImage->raw_to_ptr<void>(i), diskByte, memByte);

                success = false;
            }
        }
    }

    // (Sorta) find vfunc pointer swaps by checking relocations
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

        auto pe = shared::PE::GetImagePtr(memoryRelocPtr);

        // Reloc is supposed to point in ourselves, but it points god-knows-where?
        if (!pe)
        {
            success = false;
            MEMORY_BASIC_INFORMATION mbi{};

            if (!VirtualQuery(memoryRelocPtr, &mbi, sizeof(mbi)))
            {
                logger->info("[IntegrityChecker] Relocation {} (source {}, RVA {}) does not have a valid virtual addr!",
                             memoryRelocPtr, reinterpret_cast<void*>(memoryReloc), reloc);
                continue;
            }

            if (!shared::MemoryEnv::IsProtectionExecutable(mbi.Protect) &&
                !shared::MemoryEnv::IsProtectionExecutable(mbi.AllocationProtect))
            {
                logger->info(
                    "[IntegrityChecker] Relocation {} (source {}, RVA {}) does not point to executable memory!",
                    memoryRelocPtr, reinterpret_cast<void*>(memoryReloc), reloc);
                continue;
            }

            logger->info("[IntegrityChecker] Relocation {} (source {}, RVA {}) points outside of an image! This is "
                         "unlikely to be right.",
                         memoryRelocPtr, reinterpret_cast<void*>(memoryReloc), reloc);

            continue;
        }

        if (pe != aImage)
        {
            success = false;

            pico::UnicodeString imageName{};
            wil::GetModuleFileNameW(reinterpret_cast<HMODULE>(pe), imageName);

            logger->info(
                "[IntegrityChecker] Relocation {} (source {}, RVA {}) points to module {} instead of correct module!",
                memoryRelocPtr, reinterpret_cast<void*>(memoryReloc), reloc, shared::Util::ToUTF8(imageName));
        }

        if (aIsClientModule && !success)
        {
            break;
        }
    }

    if (!aIsClientModule)
    {
        logger->info("[IntegrityChecker] Metrics: scanned funcs {}, surface skimmed: {}", funcCoverage,
                     partialCoverage);
    }

    if (!success)
    {
        return false;
    }

    return ScanImports(aModule, aImage);
}

pico::Bool pico::Engine::IntegrityChecker::ScanImports(pico::Engine::ModuleData& aModule,
                                                       pico::shared::PE::Image* aImage) const
{
    // TODO
    // Need to resolve API sets and stuff - or just use GetModuleHandleA and GetProcAddress and pray they're not hooked
    // lol
    const auto importDirectory = aModule.m_image->get_directory(win::directory_entry_import);

    if (!importDirectory)
    {
        // Can't argue with that
        return true;
    }

    auto importDirectoryTable =
        reinterpret_cast<win::import_directory_t*>(aModule.m_image->raw_to_ptr(importDirectory->rva));

    while (importDirectoryTable->characteristics)
    {
        // auto str = aModule.m_image->raw_to_ptr<pico::Char>(importDirectoryTable->rva_name);

        // Logger::GetLogSink()->info("IAT for {} at {}, unbound: {}", str,
        //                            aModule.m_image->raw_to_ptr<void>(importDirectoryTable->rva_first_thunk),
        //                            aModule.m_image->raw_to_ptr<void>(importDirectoryTable->rva_original_first_thunk));

        importDirectoryTable++;
    }

    return true;
}

pico::Bool pico::Engine::IntegrityChecker::ScanClient()
{
    constexpr pico::Seconds ClientLibraryRefreshInterval{20};

    static auto& s_engine = Engine::Get();
    auto& s_logger = Logger::GetLogSink();

    pico::UnicodeString modulePath{};
    if (FAILED(wil::GetModuleFileNameW(reinterpret_cast<HMODULE>(s_engine.m_moduleBase), modulePath)))
    {
        // WTF?
        return false;
    }

    auto timestamp = Clock::now();

    static ModuleData s_clientData{};

    if (timestamp - s_clientData.m_lastIntegrityCheckTime > ClientLibraryRefreshInterval)
    {
        if (s_clientData.Load(modulePath))
        {
            s_clientData.m_lastIntegrityCheckTime = timestamp;
        }
        else
        {
            s_logger->error("[IntegrityChecker] Failed to refresh client module!");
            return false;
        }
    }

    return ScanModule(s_clientData, shared::PE::GetImagePtr(s_engine.m_moduleBase), true);
}

void pico::Engine::IntegrityChecker::Tick()
{
    // Very blanket, IDK how it'll work out in practice
    // Integrity checking is VERY expensive no matter how you play it
    // Additionally, we will scan the client every time
    EngineThreadLoadGuard guard{};

    shared::Util::MsTaken time{};

    constexpr pico::Seconds LoadedModuleListReportInterval{60};
    constexpr pico::Seconds ModuleIntegrityCheckInterval{20};

    auto& logger = Logger::GetLogSink();
    static const auto& s_engine = Engine::Get();

    const auto timestamp = Clock::now();

    const auto loadedModules = shared::ProcessEnv::GetLoadedModuleList();

    pico::Set<pico::Uint64> presentModules{};

    auto shouldReport = (timestamp - m_lastTimeLoadedDllsReported) > LoadedModuleListReportInterval;

    if (shouldReport)
    {
        m_lastTimeLoadedDllsReported = timestamp;
    }

    if (!ScanClient())
    {
        logger->error("[IntegrityChecker] Client failed integrity check!");
    }

    auto moduleScanned = false;

    for (auto i = 0u; i < loadedModules.size(); i++)
    {
        auto entry = loadedModules[i];
        if (shouldReport)
        {
            logger->info("[IntegrityChecker] Module {} loaded, base address {}",
                         shared::Util::ToUTF8(entry->FullDllName.Buffer), entry->DllBase);
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

        // Reset entry
        moduleEntry = {};

        if (!moduleEntry.Load(entry->FullDllName.Buffer))
        {
            logger->error("[IntegrityChecker] Failed to load module entry for {}, base address {}!",
                          shared::Util::ToUTF8(entry->BaseDllName.Buffer), entry->DllBase);
            continue;
        }

        moduleEntry.DumpModuleInfo();

        // We should not have too many consecutive integrity checks
        moduleEntry.m_lastIntegrityCheckTime = timestamp + pico::Seconds(i);

        logger->info("[IntegrityChecker] Scanning module {} at base {}",
                     shared::Util::ToUTF8(entry->BaseDllName.Buffer), entry->DllBase);

        // Why does GetImagePtr return bogus values every now and then? Who knows?
        const auto status = ScanModule(moduleEntry, reinterpret_cast<shared::PE::Image*>(entry->DllBase), false);

        if (status)
        {
            logger->info("[IntegrityChecker] Module {} at base {} passed its integrity check!",
                         shared::Util::ToUTF8(entry->BaseDllName.Buffer), entry->DllBase);
        }
        else
        {
            logger->error("[IntegrityChecker] Module {} at base {} failed its integrity check!",
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

    logger->info("[IntegrityChecker] Metrics: Time taken to run tick: {}ms", time.Now());
}

pico::Engine::IntegrityChecker& pico::Engine::IntegrityChecker::Get()
{
    static IntegrityChecker s_instance{};

    return s_instance;
}