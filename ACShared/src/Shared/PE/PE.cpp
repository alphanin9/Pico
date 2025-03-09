#include <Shared/PE/PE.hpp>
#include <Shared/ProcessEnv/ProcessEnv.hpp>

pico::shared::PE::Image* pico::shared::PE::GetImagePtr(_In_ void* const aPtrInImage)
{
    if (!aPtrInImage)
    {
        return nullptr;
    }

    pico::shared::PE::Image* peFileHeader{};

    // Note: this fails sometimes, look into it, maybe it's a concurrency issue?
    ProcessEnv::EnumerateLoadedModules(
        [&peFileHeader, aPtrInImage](Windows::LDR_DATA_TABLE_ENTRY* aEntry)
        {
            const auto asUintptr = (uintptr_t)(aPtrInImage);
            const auto dllBase = (uintptr_t)(aEntry->DllBase);

            const auto entryPe = (pico::shared::PE::Image*)(aEntry->DllBase);

            // We can skip the validity check, as invalid PE would never be loaded as a DLL
            const auto imageSize = entryPe->get_nt_headers()->optional_header.size_image;

            if (asUintptr >= dllBase && asUintptr <= dllBase + imageSize)
            {
                peFileHeader = entryPe;
                return true;
            }

            return false;
        });

    if (peFileHeader)
    {
        return peFileHeader;
    }

    // We don't know about it being a proper image? Maybe it's manual mapped or somesuch
    peFileHeader = (pico::shared::PE::Image*)(aPtrInImage);

    if (IsImageValid(peFileHeader))
    {
        return peFileHeader;
    }

    return nullptr;
}

pico::Bool pico::shared::PE::IsImageValid(_In_ const pico::shared::PE::Image* const aImage)
{
    if (!aImage)
    {
        return false;
    }

    const auto dosHeaders = aImage->get_dos_headers();

    // Is the DOS header valid?
    if (dosHeaders->e_magic != win::DOS_HDR_MAGIC)
    {
        return false;
    }

    const auto ntHeader = dosHeaders->get_nt_headers();

    // Is the PE header valid?
    return ntHeader->signature == win::NT_HDR_MAGIC;
}

std::pair<uintptr_t, uintptr_t> pico::shared::PE::GetImageBounds(_In_ const pico::shared::PE::Image* const aImage)
{
    if (!IsImageValid(aImage))
    {
        return {};
    }

    const auto& optionalHeader = aImage->get_nt_headers()->optional_header;

    const auto asUintptr = (uintptr_t)(aImage);

    // I believe it's very unlikely that we'll ever have to find a signature in PE headers
    return {asUintptr + optionalHeader.base_of_code, asUintptr + optionalHeader.size_image};
}

pico::Vector<std::pair<pico::Uint32, pico::Uint32>> pico::shared::PE::GetFunctionsOfImage(
    _In_ const pico::shared::PE::Image* const aImage)
{
    if (!IsImageValid(aImage))
    {
        return {};
    }

    const auto exceptionDir = aImage->get_directory(win::directory_entry_exception);

    if (!exceptionDir)
    {
        return {};
    }

    const auto directoryRVA = exceptionDir->rva;
    const auto directorySize = exceptionDir->size;

    const win::exception_directory exceptionDirectory{aImage->raw_to_ptr<void>(directoryRVA), directorySize};

    pico::Vector<std::pair<pico::Uint32, pico::Uint32>> ret{};

    ret.reserve(exceptionDirectory.size());

    // Grab all func bounds from directory
    for (auto functionEntry : exceptionDirectory)
    {
        ret.push_back({functionEntry.rva_begin, functionEntry.rva_end});
    }

    return ret;
}

std::pair<uintptr_t, uintptr_t> pico::shared::PE::GetFunctionBounds(_In_ const pico::shared::PE::Image* const aImage,
                                                                    _In_ const void* const aAddrInFunc)
{
    if (!IsImageValid(aImage))
    {
        return {};
    }

    const auto exceptionDir = aImage->get_directory(win::directory_entry_exception);

    if (!exceptionDir)
    {
        return {};
    }

    const auto directoryRVA = exceptionDir->rva;
    const auto directorySize = exceptionDir->size;

    const win::exception_directory exceptionDirectory{aImage->raw_to_ptr<void>(directoryRVA), directorySize};

    const auto imageBaseAsUint = (uintptr_t)(aImage);

    const auto funcRVA = aImage->ptr_to_raw(aAddrInFunc);
    const auto it = exceptionDirectory.find_overlapping(funcRVA);

    if (it == exceptionDirectory.end())
    {
        return {};
    }

    return {imageBaseAsUint + it->rva_begin, imageBaseAsUint + it->rva_end};
}

pico::Bool pico::shared::PE::IsIntegrityCheckableImageSizeSmall(_In_ const pico::shared::PE::Image* const aImage)
{
    if (!IsImageValid(aImage))
    {
        return false;
    }

    const auto ntHeaders = aImage->get_nt_headers();

    pico::Size combinedReadOnlySectionSize = 0u;

    for (auto i = 0u; i < ntHeaders->file_header.num_sections; i++)
    {
        const auto section = ntHeaders->get_section(i);

        // We only care about raw data of section

        if (section->characteristics.mem_write == 0)
        {
            combinedReadOnlySectionSize += section->size_raw_data;
        }
    }

    constexpr pico::Size MinimumIntegrityCheckableValidSectionSize = 0x4000;

    return combinedReadOnlySectionSize <= MinimumIntegrityCheckableValidSectionSize;
}

pico::Bool pico::shared::PE::HasLargeRWXSections(_In_ const pico::shared::PE::Image* const aImage)
{
    if (!IsImageValid(aImage))
    {
        return false;
    }

    const auto ntHeaders = aImage->get_nt_headers();

    pico::Size totalRwxSectionSize = 0u;

    for (auto i = 0u; i < ntHeaders->file_header.num_sections; i++)
    {
        const auto section = ntHeaders->get_section(i);

        // Nice, RWX section found
        if (section->characteristics.mem_read && section->characteristics.mem_write &&
            section->characteristics.mem_execute)
        {
            // Here we only care about virtual size
            totalRwxSectionSize += section->virtual_size;
        }
    }

    constexpr pico::Size MaxSizeOfRWXSection = 0x4000;

    return totalRwxSectionSize >= MaxSizeOfRWXSection;
}

pico::String pico::shared::PE::GetImagePDBPath(_In_ const pico::shared::PE::Image* const aImage)
{
    if (!IsImageValid(aImage))
    {
        return {};
    }

    const auto debugDirectoryInfo = aImage->get_directory(win::directory_entry_debug);

    if (!debugDirectoryInfo)
    {
        return {};
    }

    const auto debugDirectory = aImage->raw_to_ptr<win::debug_directory_t>(debugDirectoryInfo->rva);

    for (auto i = 0u; i < (debugDirectoryInfo->size / sizeof(win::debug_directory_entry_t)); i++)
    {
        auto& entry = debugDirectory->entries[i];

        if (entry.type != win::debug_directory_type_id::codeview)
        {
            continue;
        }

        const auto header = aImage->raw_to_ptr<win::cv_header_t>(entry.rva_raw_data);

        if (header->signature != win::cv_signature::pdb70)
        {
            continue;
        }

        const auto pdbData = aImage->raw_to_ptr<win::cv_pdb70_t>(entry.rva_raw_data);

        // Char list length is zero here
        const auto textSize = entry.size_raw_data - sizeof(win::cv_pdb70_t);

        pico::String ret{&pdbData->pdb_name[0], &pdbData->pdb_name[textSize]};

        return ret;
    }

    // TODO

    return {};
}

pico::Vector<pico::Uint32> pico::shared::PE::GetRelocations(_In_ const pico::shared::PE::Image* const aImage)
{
    if (!IsImageValid(aImage))
    {
        return {};
    }

    const auto ntHeaders = aImage->get_nt_headers();

    const auto relocationsDirRva = ntHeaders->optional_header.data_directories.basereloc_directory;

    if (!relocationsDirRva.present())
    {
        return {};
    }

    const auto relocationsDirectory = aImage->raw_to_ptr<win::reloc_directory_t>(relocationsDirRva.rva);

    pico::Vector<pico::Uint32> relocs{};

    for (const auto* relocBlock = &relocationsDirectory->first_block; relocBlock->base_rva;
         relocBlock = relocBlock->next())
    {
        for (const auto relocEntry : *relocBlock)
        {
            const auto totalRva = relocBlock->base_rva + relocEntry.offset;
            // Note: for our usecase, we only care about IMAGE_REL_BASED_DIR64 - while some other things will use
            // different relocation types, we don't really care
            if (relocEntry.type == win::rel_based_dir64)
            {
                relocs.push_back(totalRva);
            }
        }
    }

    return relocs;
}