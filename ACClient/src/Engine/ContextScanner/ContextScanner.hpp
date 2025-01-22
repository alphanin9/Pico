#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{
/**
 * \brief Frame for thread context scanner. Contains necessary data.
 */
struct ContextFrame
{
    // Thread ID
    pico::Uint32 m_threadId{};

    // Instruction pointer
    pico::Uint64 m_rip;

    // Stack pointer
    pico::Uint64 m_rsp;

    // Page stack pointer was on
    pico::Vector<void*> m_stackPage{};
};

/**
 * \brief Thread context scanner detection module.
 *
 * Detection component is intended to run in main thread, stack trace checker will sit in thread pool.
 *
 * Overview:
 *
 * Scanner gets pinged, randomly selects thread (avoiding our thread pool threads), suspends it,
 * dumps RIP and the current page the stack is on into vector, continues thread
 *
 * Thread pool worker swaps buffers, iterates, logs bad things
 *
 * Note: maybe move pinging scanner to thread pool?
 * At the moment this won't be too effective at catching code running in hook
 *
 * Additionally, VirtualQuery of 512 potential vaddrs will be slow - even with heuristics
 * 
 * Note: this can also scan stacks obtained from other sources
 */
struct ContextScanner : public shared::Util::NonCopyableOrMovable
{
    // Whether or not the component is currently running its tick function. (TODO)
    pico::AtomicBool m_isExecuting{};

    std::mutex m_contextScannerMutex{};

    // Buffer for received context frames before they can be processed
    pico::Vector<pico::SharedPtr<ContextFrame>> m_receivedFrames{};

    // Buffer for context frames in processing
    pico::Vector<pico::SharedPtr<ContextFrame>> m_scannedFrames{};

    /**
     * \brief Tick component in main thread.
     */
    void TickMainThread() noexcept;

    /**
     * \brief Ticks component in thread pool worker thread.
     */
    void Tick() noexcept;

    /**
     * \brief Locks the mutex and pushes a frame to the context frame list.
     * \param aFrame A reference to a shared ptr of a filled context frame.
     */
    void PushFrame(pico::SharedPtr<ContextFrame>& aFrame) noexcept;

    /**
     * \brief Get a singleton instance of the context scanner.
     * \return A singleton instance of the context scanner.
     */
    static ContextScanner& Get() noexcept;
};
} // namespace pico::Engine