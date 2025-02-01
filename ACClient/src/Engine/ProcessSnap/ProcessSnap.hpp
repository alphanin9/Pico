#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{
/**
 * \brief A more lightweight wrapper over Windows::SYSTEM_EXTENDED_THREAD_INFORMATION, containing things we need.
 * 
 * This is currently not displayed in the logs due to performance concerns.
 * 
 * Note that some things may be zero.
 */
struct ThreadInformation
{
    pico::Uint32 m_threadId{};
    
    pico::Uint64 m_userTime{};
    pico::Uint64 m_kernelTime{};

    pico::Uint64 m_startAddress{};
    pico::Uint64 m_win32StartAddress{};

    pico::Uint64 m_tebAddr{};

    Windows::KTHREAD_STATE m_threadState{};
    Windows::KWAIT_REASON m_waitReason{};

    ThreadInformation(Windows::SYSTEM_EXTENDED_THREAD_INFORMATION* aThread) noexcept;
    ThreadInformation() = default;
};

/**
 * \brief A more lightweight wrapper over Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION, containing things we need.
 */
struct ProcessInformation
{
    pico::Uint32 m_processId{};
    pico::Uint32 m_sessionId{};
    // Note: this is only the image name itself, not the whole path!
    pico::UnicodeString m_imageName{};
    pico::Uint64 m_imageNameHash{}; 

    pico::UnicodeString m_imagePath{};

    pico::Vector<ThreadInformation> m_threads{};

    ProcessInformation(Windows::SYSTEM_EXTENDED_PROCESS_INFORMATION* aProc,
                       const pico::Vector<Windows::SYSTEM_EXTENDED_THREAD_INFORMATION*>& aThreads) noexcept;

    ProcessInformation() = default;
};

/**
 * \brief Process enumeration to catch potential blacklisted applications or gather details on external hacks that have
 * handles opened to us.
 *
 * Dumps some information about (mostly) every process open on the system (PID, full path, running threads...)
 * In the future, can be extended to dump more information about a process (PE structure, debug symbol path...)
 */
struct ProcessSnap : public shared::Util::NonCopyableOrMovable
{
    // How often the process cache should be refreshed.
    static constexpr pico::Seconds ProcessCacheRefreshTime{15};

    // The last time the process cache was refreshed.
    pico::Timestamp m_lastProcessRefreshTime{};

    pico::Vector<ProcessInformation> m_processInfo{};

    pico::Size m_lastIndex{};
    pico::Bool m_isDone{};

    // Whether or not the component is currently running its tick function. (TODO)
    pico::AtomicBool m_isExecuting{};

    /**
     * \brief Tick component in thread pool worker.
     */
    void Tick() noexcept;

    /**
     * \brief Get a singleton instance of the process snapper.
     * \return A singleton instance of the process snapper.
     */
    static ProcessSnap& Get() noexcept;
};
}
