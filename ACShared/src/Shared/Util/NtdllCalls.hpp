#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>
/*
 * Due to some NTDLL calls requiring you to use stuff from winternl.h,
 * I made a few wrappers to use our types from NTDLL PDB dump
 * We import the func with LazyImporter
 */

namespace Windows
{
NTSTATUS __stdcall NtQuerySystemInformation(Windows::SYSTEM_INFORMATION_CLASS aClass, _Out_ void* aSystemInformation,
                                            pico::Uint32 aSizeOfSystemInformation,
                                            _Out_ pico::Uint32& aSizeWritten) noexcept;

NTSTATUS __stdcall NtQueryInformationProcess(HANDLE aHandle, Windows::PROCESSINFOCLASS aClass,
                                             _Out_ void* aProcInformation, pico::Uint32 aSizeOfProcInformation,
                                             _Out_ pico::Uint32& aSizeWritten) noexcept;

NTSTATUS __stdcall NtQueryInformationThread(HANDLE aHandle, Windows::THREADINFOCLASS aClass,
                                            _Out_ void* aThreadInformation, pico::Uint32 aSizeOfThreadInformation,
                                            _Out_ pico::Uint32& aSizeWritten) noexcept;

NTSTATUS __stdcall NtQueryVirtualMemory(HANDLE aHandle, uintptr_t aBaseAddress,
                                        Windows::MEMORY_INFORMATION_CLASS aClass, _Out_ void* aMemoryInformation,
                                        pico::Uint32 aSizeOfMemoryInformation,
                                        _Out_ pico::Uint32& aSizeWritten) noexcept;

NTSTATUS __stdcall NtAllocateVirtualMemory(HANDLE aHandle, uintptr_t& aBaseAddress, uintptr_t aZeroBits,
                                           pico::Size& aRegionSize, pico::Uint32 aAllocationType,
                                           pico::Uint32 aProtection) noexcept;

NTSTATUS __stdcall NtFreeVirtualMemory(HANDLE aHandle, uintptr_t& aBaseAddress, pico::Size& aSize,
                                       pico::Uint32 aFreeType) noexcept;

NTSTATUS __stdcall NtProtectVirtualMemory(HANDLE aHandle, uintptr_t& aBaseAddress, pico::Size& aSize,
                                          pico::Uint32 aNewProtection, pico::Uint32& aOldProtection) noexcept;

NTSTATUS __stdcall NtCreateFile(_Out_ HANDLE& aFileHandle, ACCESS_MASK aDesiredAccess, _In_ Windows::OBJECT_ATTRIBUTES* aObjAttributes,
                                _In_ Windows::IO_STATUS_BLOCK* aStatusBlock, Windows::LARGE_INTEGER* aAllocationSize,
                                pico::Uint32 aAttributes, pico::Uint32 aFileShareMode, pico::Uint32 aCreateDisposition,
                                pico::Uint32 aCreateOptions, void* aEaBuffer, pico::Uint32 aEaLength) noexcept;


NTSTATUS __stdcall NtReadVirtualMemory(HANDLE aProcessHandle, uintptr_t aBaseAddress, _Out_ void* aBuffer,
                                       pico::Size aSize, pico::Size& aSizeRead) noexcept;

void __stdcall RtlInitUnicodeString(_Out_ Windows::UNICODE_STRING& aStr, _In_ const pico::WChar* aSource) noexcept;

NTSTATUS __stdcall NtDuplicateObject(_In_ HANDLE aSourceProcHandle, _In_ HANDLE aSourceHandle,
                                     HANDLE aTargetProcessHandle, _Out_ HANDLE& aTargetHandle,
                                     ACCESS_MASK aDesiredAccess, pico::Uint32 aHandleAttributes, pico::Uint32 aOptions) noexcept;

NTSTATUS __stdcall NtQueryObject(_In_ HANDLE aObjectHandle, Windows::OBJECT_INFORMATION_CLASS aObjectInfoClass,
                                 _Out_ void* aObjectInfo, pico::Uint32 aObjectInfoSize,
                                 pico::Uint32& aSizeWritten) noexcept;
} // namespace Windows 