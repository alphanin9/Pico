#include <Shared/ProcessEnv/MemoryEnv.hpp>

#include <lazy_importer.hpp>

pico::Bool pico::shared::MemoryEnv::FillProcessWorkingSetBuffer(pico::Vector<pico::Uint8>& aBuffer) noexcept
{
    constexpr pico::Size MinimumBufferSize = 0x8000u;

    // Resize the buffer to a minimum worthwhile size
    // If it's already in a good size, clear it
    aBuffer.assign(std::max(aBuffer.size(), MinimumBufferSize), {});

    pico::Uint32 sizeWritten{};

    // For some reason it fails when current process handle is static?
    // I blame some sort of probing in kernel
    const auto currentProcess = GetCurrentProcess();

    auto status = Windows::NtQueryVirtualMemory(currentProcess, 0u, Windows::MemoryWorkingSetInformation,
                                                aBuffer.data(), aBuffer.size(), sizeWritten);

    // Fun fact: this actually uses STATUS_ACCESS_VIOLATION for reporting the buffer is too small instead of
    // STATUS_INFO_LENGTH_MISMATCH
    // Windows consistency
    // It uses both
    while (status == STATUS_ACCESS_VIOLATION || status == STATUS_INFO_LENGTH_MISMATCH)
    {
        auto asWorkingSetInformation = reinterpret_cast<Windows::MEMORY_WORKING_SET_INFORMATION*>(&aBuffer[0]);

        auto sizeNeeded = (asWorkingSetInformation->NumberOfEntries * sizeof(Windows::MEMORY_WORKING_SET_BLOCK)) +
                          (sizeof(asWorkingSetInformation->NumberOfEntries));

        aBuffer.assign(sizeNeeded, {});

        status = Windows::NtQueryVirtualMemory(currentProcess, 0u, Windows::MemoryWorkingSetInformation, aBuffer.data(),
                                               aBuffer.size(), sizeWritten);
    }

    return NT_SUCCESS(status);
}

pico::Vector<Windows::MEMORY_WORKING_SET_BLOCK> pico::shared::MemoryEnv::GetProcessWorkingSet() noexcept
{
    pico::Vector<pico::Uint8> buffer{};

    if (!pico::shared::MemoryEnv::FillProcessWorkingSetBuffer(buffer))
    {
        return {};
    }

    auto asWorkingSetInformation = reinterpret_cast<Windows::MEMORY_WORKING_SET_INFORMATION*>(&buffer[0]);

    pico::Vector<Windows::MEMORY_WORKING_SET_BLOCK> workingSet{};

    workingSet.reserve(asWorkingSetInformation->NumberOfEntries);

    for (auto i = 0u; i < asWorkingSetInformation->NumberOfEntries; i++)
    {
        workingSet.push_back(asWorkingSetInformation->WorkingSetInfo[i]);
    }

    return workingSet;
}

pico::Bool pico::shared::MemoryEnv::IsProtectionExecutable(pico::Uint32 aProtection) noexcept
{
    return aProtection == PAGE_EXECUTE || aProtection == PAGE_EXECUTE_READ || aProtection == PAGE_EXECUTE_READWRITE ||
           aProtection == PAGE_EXECUTE_WRITECOPY;
}
