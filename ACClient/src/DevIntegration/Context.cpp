#include <DevIntegration/Context.hpp>

pico::Integration::Context::Context()
{
    m_msvcLogSink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    m_logger = std::make_shared<spdlog::logger>("Integration", m_msvcLogSink);

    m_logger->flush_on(spdlog::level::trace);
}

pico::Integration::Context& pico::Integration::Context::Get()
{
    static pico::Integration::Context s_ctx{};

    return s_ctx;
}