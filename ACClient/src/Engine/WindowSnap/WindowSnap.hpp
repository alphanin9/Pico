#pragma once
#include <Shared/Pico.hpp>

namespace pico::Engine
{
/**
 * \brief Window enumeration. Enumerates currently opened windows and dumps their names, class names, flags and owner
 * process IDs.
 *
 * This may be somewhat too invasive for usage, but it should work for seeing overlays.
 */
struct WindowSnap : public shared::Util::NonCopyableOrMovable
{
    pico::Size m_nextWindowIndex{};
    pico::Vector<HWND> m_windowCache{};

    void Tick();

    static WindowSnap& Get();
};
} // namespace pico::Engine
