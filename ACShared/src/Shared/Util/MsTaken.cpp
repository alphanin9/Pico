#include <Shared/Util/MsTaken.hpp>

pico::shared::Util::MsTaken::MsTaken() noexcept
    : m_start(pico::Clock::now())
{
    
}

pico::Uint64 pico::shared::Util::MsTaken::Now() const noexcept
{
    return std::chrono::duration_cast<pico::Milliseconds>(pico::Clock::now() - m_start).count();
}
