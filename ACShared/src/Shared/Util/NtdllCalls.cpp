#include <Shared/Util/NtdllCalls.hpp>
#include <lazy_importer.hpp>

// Optimally we'd use LI/our own exports iterator to get address,
// disassemble the system call stub (if applicable) to get syscall index
// then codegen our own system calls using now known syscall index
// (cons: JIT is evil for our purpose... but we use it anyway!)
// Or have an assembly stub setting things up for direct syscall after getting syscall indices

NTSTATUS __stdcall Windows::NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS aClass,
                                                     _Out_ void* aSystemInformation,
                                                     pico::Uint32 aSizeOfSystemInformation,
                                                     _Out_ pico::Uint32& aSizeWritten)
{
    static const auto s_func = LI_FN(NtQuerySystemInformation).nt_cached();

    return s_func(aClass, aSystemInformation, aSizeOfSystemInformation, aSizeWritten);
}

NTSTATUS __stdcall Windows::NtQueryInformationProcess(HANDLE aHandle, Windows::PROCESSINFOCLASS aClass,
                                                      _Out_ void* aProcInformation, pico::Uint32 aSizeOfProcInformation,
                                                      _Out_ pico::Uint32& aSizeWritten)
{
    static const auto s_func = LI_FN(NtQueryInformationProcess).nt_cached();

    return s_func(aHandle, aClass, aProcInformation, aSizeOfProcInformation, aSizeWritten);
}

NTSTATUS __stdcall Windows::NtQueryInformationThread(HANDLE aHandle, Windows::THREADINFOCLASS aClass,
                                                     _Out_ void* aThreadInformation,
                                                     pico::Uint32 aSizeOfThreadInformation,
                                                     _Out_ pico::Uint32& aSizeWritten)
{
    static const auto s_func = LI_FN(NtQueryInformationThread).nt_cached();

    return s_func(aHandle, aClass, aThreadInformation, aSizeOfThreadInformation, aSizeWritten);
}

NTSTATUS __stdcall Windows::NtQueryVirtualMemory(HANDLE aHandle, uintptr_t aBaseAddress,
                                                 Windows::MEMORY_INFORMATION_CLASS aClass,
                                                 _Out_ void* aMemoryInformation, pico::Uint32 aSizeOfMemoryInformation,
                                                 _Out_ pico::Uint32& aSizeWritten)
{
    static const auto s_func = LI_FN(NtQueryVirtualMemory).nt_cached();

    return s_func(aHandle, aBaseAddress, aClass, aMemoryInformation, aSizeOfMemoryInformation, aSizeWritten);
}

NTSTATUS __stdcall Windows::NtAllocateVirtualMemory(HANDLE aHandle, uintptr_t& aBaseAddress, uintptr_t aZeroBits,
                                                    pico::Size& aRegionSize, pico::Uint32 aAllocationType,
                                                    pico::Uint32 aProtection)
{
    static const auto s_func = LI_FN(NtAllocateVirtualMemory).nt_cached();

    // We should also be setting the preferred NUMA node here, but it does not matter here
    aAllocationType &= 0xFFFFFFC0;

    return s_func(aHandle, aBaseAddress, aZeroBits, aRegionSize, aAllocationType, aProtection);
}

NTSTATUS __stdcall Windows::NtFreeVirtualMemory(HANDLE aHandle, uintptr_t& aBaseAddress, pico::Size& aSize,
                                                pico::Uint32 aFreeType)
{
    static const auto s_func = LI_FN(NtFreeVirtualMemory).nt_cached();

    return s_func(aHandle, aBaseAddress, aSize, aFreeType);
}

NTSTATUS __stdcall Windows::NtProtectVirtualMemory(HANDLE aHandle, uintptr_t& aBaseAddress, pico::Size& aSize,
                                                   pico::Uint32 aNewProtection, pico::Uint32& aOldProtection)
{
    static const auto s_func = LI_FN(NtProtectVirtualMemory).nt_cached();

    return s_func(aHandle, aBaseAddress, aSize, aNewProtection, aOldProtection);
}

NTSTATUS __stdcall Windows::NtCreateFile(_Out_ HANDLE& aFileHandle, ACCESS_MASK aDesiredAccess,
                                         _In_ Windows::OBJECT_ATTRIBUTES* aObjAttributes,
                                         _In_ Windows::IO_STATUS_BLOCK* aStatusBlock,
                                         Windows::LARGE_INTEGER* aAllocationSize, pico::Uint32 aAttributes,
                                         pico::Uint32 aFileShareMode, pico::Uint32 aCreateDisposition,
                                         pico::Uint32 aCreateOptions, void* aEaBuffer, pico::Uint32 aEaLength)
{
    static const auto s_func = LI_FN(NtCreateFile).nt_cached();

    return s_func(aFileHandle, aDesiredAccess, aObjAttributes, aStatusBlock, aAllocationSize, aAttributes,
                  aFileShareMode, aCreateDisposition, aCreateOptions, aEaBuffer, aEaLength);
}

NTSTATUS __stdcall Windows::NtReadVirtualMemory(HANDLE aProcessHandle, uintptr_t aBaseAddress, _Out_ void* aBuffer,
                                                pico::Size aSize, pico::Size& aSizeRead)
{
    static const auto s_func = LI_FN(NtReadVirtualMemory).nt_cached();

    return s_func(aProcessHandle, aBaseAddress, aBuffer, aSize, aSizeRead);
}

void __stdcall Windows::RtlInitUnicodeString(_Out_ Windows::UNICODE_STRING& aStr, _In_ const pico::WChar* aSource)
{
    static const auto s_func = LI_FN(RtlInitUnicodeString).nt_cached();

    return s_func(aStr, aSource);
}

NTSTATUS __stdcall Windows::NtDuplicateObject(_In_ HANDLE aSourceProcHandle, _In_ HANDLE aSourceHandle,
                                              HANDLE aTargetProcessHandle, _Out_ HANDLE& aTargetHandle,
                                              ACCESS_MASK aDesiredAccess, pico::Uint32 aHandleAttributes,
                                              pico::Uint32 aOptions)
{
    static const auto s_func = LI_FN(NtDuplicateObject).nt_cached();

    return s_func(aSourceProcHandle, aSourceHandle, aTargetProcessHandle, aTargetHandle, aDesiredAccess,
                  aHandleAttributes, aOptions);
}

NTSTATUS __stdcall Windows::NtQueryObject(_In_ HANDLE aObjectHandle, Windows::OBJECT_INFORMATION_CLASS aObjectInfoClass,
                                          _Out_ void* aObjectInfo, pico::Uint32 aObjectInfoSize,
                                          pico::Uint32& aSizeWritten)
{
    static const auto s_func = LI_FN(NtQueryObject).nt_cached();

    return s_func(aObjectHandle, aObjectInfoClass, aObjectInfo, aObjectInfoSize, aSizeWritten);
}

NTSTATUS __stdcall Windows::NtSetInformationProcess(_In_ HANDLE aProcessHandle, Windows::PROCESSINFOCLASS aClass,
                                                    _In_opt_ void* aProcInformation,
                                                    pico::Uint32 aSizeOfProcInformation)
{
    static const auto s_func = LI_FN(NtSetInformationProcess).nt_cached();

    return s_func(aProcessHandle, aClass, aProcInformation, aSizeOfProcInformation);
}