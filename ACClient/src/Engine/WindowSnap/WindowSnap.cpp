#include <Engine/Logging/Logger.hpp>
#include <Engine/WindowSnap/WindowSnap.hpp>

void pico::Engine::WindowSnap::Tick()
{
    auto& logger = Logger::GetLogSink();

    if (m_windowCache.empty())
    {
        shared::Util::MsTaken windowGrab{};
        EnumWindows(
            [](HWND aHwnd, LPARAM aThis) -> BOOL
            {
                auto self = (WindowSnap*)(aThis);

                self->m_windowCache.push_back(aHwnd);

                return TRUE;
            },
            (LPARAM)(this));

        logger->info("[WindowSnap] Window count: {}, taken: {}ms", m_windowCache.size(), windowGrab.Now());

        m_nextWindowIndex = 0u;
    }

    // The .at() is intentional
    auto window = m_windowCache.at(m_nextWindowIndex);

    WINDOWINFO info{};

    info.cbSize = sizeof(info);

    if (GetWindowInfo(window, &info))
    {
        pico::Uint32 pid{};
        GetWindowThreadProcessId(window, (LPDWORD)(&pid));

        pico::UnicodeString processImage{};

        wil::unique_handle proc{OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid)};

        if (proc)
        {
            wil::QueryFullProcessImageNameW(proc.get(), 0u, processImage);
        }

        constexpr pico::Size WindowTextSize = 250u;

        pico::String windowTitle(WindowTextSize, {});
        pico::String windowClass(WindowTextSize, {});

        if (GetWindowTextA(window, windowTitle.data(), (pico::Int32)(windowTitle.size())) == 0)
        {
            windowTitle = "<none>";
        }

        if (GetClassNameA(window, windowClass.data(), (pico::Int32)(windowClass.size())))
        {
            windowClass = "<none>";
        }

        logger->info("[WindowSnap] Window {} (PID {}, image {})", (void*)(window), pid,
                     !processImage.empty() ? shared::Util::ToUTF8(processImage) : "<none>");

        logger->info("[WindowSnap] Title: {} ({:#x}), class: {} ({:#x})", pico::StringView(windowTitle.c_str()),
                     shared::FNV1a64(windowTitle.c_str()), pico::StringView(windowClass.c_str()),
                     shared::FNV1a64(windowClass.c_str()));

        logger->info("[WindowSnap] Bounds (x, y, w, h): ({}, {}, {}, {})", info.rcWindow.left, info.rcWindow.top,
                     info.rcWindow.right - info.rcWindow.left, info.rcWindow.bottom - info.rcWindow.top);

        logger->info("[WindowSnap] Style: {:#x}, extended style: {:#x}", info.dwStyle, info.dwExStyle);

        if ((info.dwExStyle & (WS_EX_TRANSPARENT | WS_EX_TOPMOST)) == (WS_EX_TRANSPARENT | WS_EX_TOPMOST))
        {
            logger->warn("[WindowSnap] Window {} has suspicious attributes!", (void*)(window));
        }
    }

    m_nextWindowIndex++;

    if (m_nextWindowIndex == m_windowCache.size())
    {
        m_windowCache.clear();
    }
}

pico::Engine::WindowSnap& pico::Engine::WindowSnap::Get()
{
    static WindowSnap s_instance{};

    return s_instance;
}