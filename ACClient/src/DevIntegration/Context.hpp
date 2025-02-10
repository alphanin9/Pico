#pragma once
#include <Shared/Pico.hpp>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>

namespace pico::Integration
{
struct Context : shared::Util::NonCopyableOrMovable
{
    std::shared_ptr<spdlog::sinks::msvc_sink_mt> m_msvcLogSink{};
    std::shared_ptr<spdlog::logger> m_logger{};

    Context();

    /**
     * \brief Gets an instance of the Integration::Context structure.
     * \return An instance of the Integration::Context structure.
     */
    static Context& Get();
};
} // namespace pico::Integration