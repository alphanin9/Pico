#pragma once
#include <Shared/Common.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>
#include <Shared/Util/HashedString.hpp>

namespace pico::shared::ProcessEnv
{
/**
* \brief Gets a pointer to the process environment block.
* \returns A pointer to the process environment block.
*/
Windows::PEB* GetProcessEnvironment() noexcept;

/**
 * \brief Gets a pointer to the current thread's environment block.
 * \returns A pointer to the current thread's environment block.
 */
Windows::TEB64* GetCurrentThreadEnvironment() noexcept;

/**
* \brief Enumerates all loaded modules and returns a vector of them.
* \returns A vector of the current loaded modules.
*/
pico::Vector<Windows::LDR_DATA_TABLE_ENTRY*> GetLoadedModuleList() noexcept;

/**
 * \brief Enumerates loaded modules. If the module with a case-insensitive FNV1a64 hash of aHash is present, returns a pointer to it.
 * \param aHash A case-insensitive FNV1a64 hash of the desired module's file name.
 * \returns The pointer to the specified module if it's loaded. If the module is not loaded, returns nullptr.
 */
Windows::LDR_DATA_TABLE_ENTRY* GetModuleByHash(pico::Uint64 aHash) noexcept;

/**
 * \brief Enumerates loaded modules. If the module with the corresponding file name is present, returns a pointer to it. 
 * \param aHash The desired module's file name. 
 * \returns The pointer to the specified module if it's loaded. If the module is not loaded, returns nullptr.
 */
Windows::LDR_DATA_TABLE_ENTRY* GetModuleByName(pico::shared::HashedString aHash) noexcept;

/**
* \brief Enumerates modules loaded into current process.
* 
* \tparam Fn Type of function. Function signature should be pico::Bool(Windows::LDR_DATA_TABLE_ENTRY*);
* \param aCallback Callback function called on each module entry. If the callback function returns true, the loop is ended. 
*/
template<typename Fn>
inline void EnumerateLoadedModules(Fn&& aCallback) noexcept
{
    static const auto s_peb = GetProcessEnvironment();

    // Iterate over loaded modules
    for (auto entry = s_peb->Ldr->InMemoryOrderModuleList.Flink; entry != s_peb->Ldr->InMemoryOrderModuleList.Blink;
         entry = entry->Flink)
    {
        // This should be const, but it makes usage weirder in callbacks
        auto moduleData = CONTAINING_RECORD(entry, Windows::LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (aCallback(moduleData))
        {
            break;
        }
    }
}
}