#include <Shared/ProcessEnv/MemoryEnv.hpp>

#include <lazy_importer.hpp>

pico::Vector<Windows::MEMORY_WORKING_SET_BLOCK> pico::shared::MemoryEnv::GetProcessWorkingSet() noexcept
{
    constexpr auto BufferSize = 0x8000;

    // Create buffer for NtQueryVirtualMemory info
    pico::Vector<pico::Uint8> buffer(BufferSize, {});
    pico::Uint32 sizeWritten{};

    const auto currentProc = GetCurrentProcess();

    auto status = Windows::NtQueryVirtualMemory(currentProc, 0u, Windows::MemoryWorkingSetInformation, buffer.data(),
                                                buffer.size(), sizeWritten);

    // Fun fact: this actually uses STATUS_ACCESS_VIOLATION for reporting the buffer is too small instead of
    // STATUS_INFO_LENGTH_MISMATCH
    // Windows consistency
    // It uses both
    while (status == STATUS_ACCESS_VIOLATION || status == STATUS_INFO_LENGTH_MISMATCH)
    {
        auto asWorkingSetInformation = reinterpret_cast<Windows::MEMORY_WORKING_SET_INFORMATION*>(&buffer[0]);

        auto sizeNeeded = (asWorkingSetInformation->NumberOfEntries * sizeof(Windows::MEMORY_WORKING_SET_BLOCK)) +
                          (sizeof(asWorkingSetInformation->NumberOfEntries));

        buffer.assign(sizeNeeded, {});

        status = Windows::NtQueryVirtualMemory(currentProc, 0u, Windows::MemoryWorkingSetInformation,
                                               buffer.data(), buffer.size(), sizeWritten);
    }

    if (!NT_SUCCESS(status))
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
