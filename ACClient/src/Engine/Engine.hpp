#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{
/**
 * \brief Provides core functionality for the protection.
 */
struct Engine : shared::Util::NonCopyableOrMovable
{
    // Counter to track how many threads in the thread pool are under heavy load that should not be waited for
    // Should never go below 0
    // Not sure about the design, makes it easy to break
    std::atomic_int m_threadsUnderHeavyLoad{};

    // Whether or not we've initialized
    pico::Bool m_hasRunPreflight{};

    // Client module base
    void* m_moduleBase{};

    // Client module size
    pico::Size m_moduleSize{};
    // Note: These should be refreshed and verified every now and then as well

    // Basic values for utility
    // Page size on the system
    pico::Uint32 m_pageSize{};

    // Minimum UM address
    pico::Uint64 m_minimumUMAddress{};

    // Maximum UM address
    pico::Uint64 m_maximumUMAddress{};

    /**
     * \brief Get various information about the system that will not change during execution.
     * (example: Secure Boot state, code integrity state...). In addition, set up various
     * important variables.
     * 
     * In a production environment, if checks here report bad data, the application should not launch.
     * Most of these checks should not be inside of the client, but inside of the launcher
     * or be done driver-side.
     */
    void Setup() noexcept;

    /**
     * \brief Load important information about the client module.
     * In production, this routine needs to be done differently
     * as it's unlikely the client will be loaded as a normal shared library.
     */
    void SetupModuleData() noexcept;

    /**
     * \brief Tick thread pool and add new tasks.
     */
    void Tick() noexcept;

    /**
     * \brief Tick jobs that should be done on the main thread.
     * 
     * Example: thread pool checks,
     * maybe some things related to thread context checking to catch stray RIP,
     * maybe setting instrumentation callback...
     */
    void TickMainThreadJobs() noexcept;

    /**
     * \brief Checks if the thread pool has been tampered with.
     * 
     * Checks if jobs execute fine and can be waited for, as well as checking for thread suspensions.
     * 
     * \return Whether or not the pool has issues.
     */
    pico::Bool IsThreadPoolOK() noexcept;

    /**
     * \brief Checks if an address belongs to our module. This should be inlined.
     * 
     * \param aAddy The checked address.
     * \return True if the address is within our module.
     */
    __forceinline pico::Bool IsAddressInUs(uintptr_t aAddy) const noexcept
    {
        return aAddy >= reinterpret_cast<uintptr_t>(m_moduleBase) &&
               aAddy <= reinterpret_cast<uintptr_t>(m_moduleBase) + m_moduleSize;
    }

    /**
     * \brief Get a singleton instance of the engine.
     * \return A singleton instance of the engine.
     */
    static Engine& Get() noexcept;
};

/**
 * \brief A helper structure to manage Engine::m_threadsUnderHeavyLoad.
 * 
 * When constructed, increments Engine::m_threadsUnderHeavyLoad by one.
 * 
 * When destructed, decrements Engine::m_threadsUnderHeavyLoad by one.
 */
struct EngineThreadLoadGuard : shared::Util::NonCopyable
{
    EngineThreadLoadGuard() noexcept;
    ~EngineThreadLoadGuard() noexcept;
};
}