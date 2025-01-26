#include <Engine/Logging/Logger.hpp>

#include <basen.hpp>
#include <uuid.h>

pico::Engine::Logger::Logger() noexcept
    : m_dumpFolderName(std::format("PicoLogs_{:%Y%m%d%H%M%S}", std::chrono::system_clock::now()).c_str())
{
    spdlog::set_level(spdlog::level::trace);

    m_debugStringSink = std::make_shared<spdlog::sinks::msvc_sink_mt>(true);
    m_fileSink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(std::format("{}/Pico.log", m_dumpFolderName.c_str()));

    m_logger = std::make_shared<spdlog::logger>("Pico");

    m_logger->sinks().push_back(m_debugStringSink);
    m_logger->sinks().push_back(m_fileSink);

    spdlog::register_logger(m_logger);
    spdlog::set_default_logger(m_logger);

    m_logger->flush_on(spdlog::level::err);
}

void pico::Engine::Logger::DumpDataToFile(pico::StringView aFileNameTag, void* aDataStart,
                                          pico::Size aDataSize) noexcept
{
    const auto uuid = uuids::to_string(uuids::uuid_system_generator{}());
    const auto time = std::chrono::system_clock::now();

    // ./PicoLogs_{datetime}/{current_datetime}_{tag}_{uuid}.bin
    auto path = std::format("./{}/{:%Y%m%d%H%M%S}_{}_{}.bin", m_dumpFolderName.c_str(), time, aFileNameTag, uuid);
    auto base64Path =
        std::format("./{}/{:%Y%m%d%H%M%S}_{}_{}_BASE64.txt", m_dumpFolderName.c_str(), time, aFileNameTag, uuid);

    m_logger->info("[Logger] Dumping {} bytes of data to {}...", aDataSize, path);

    std::ofstream file{path, std::ios::binary};
    std::ofstream base64File{base64Path, std::ios::binary};

    pico::String base64{};

    bn::encode_b64(reinterpret_cast<pico::Uint8*>(aDataStart), reinterpret_cast<pico::Uint8*>(aDataStart) + aDataSize,
                   std::back_inserter(base64));

    file.write(reinterpret_cast<const char*>(aDataStart), aDataSize);
    base64File.write(base64.data(), base64.size());

    m_logger->info("[Logger] Wrote {} bytes to file {}", static_cast<pico::Int64>(file.tellp()), path);
    m_logger->info("[Logger] Wrote {} bytes to Base64 file {}", static_cast<pico::Int64>(base64File.tellp()), path);
}

void pico::Engine::Logger::Tick() noexcept
{
    m_logger->flush();
}

pico::Engine::Logger& pico::Engine::Logger::Get() noexcept
{
    static Logger s_instance{};

    return s_instance;
}

std::shared_ptr<spdlog::logger>& pico::Engine::Logger::GetLogSink() noexcept
{
    static auto& s_logger = Get();

    return s_logger.m_logger;
}