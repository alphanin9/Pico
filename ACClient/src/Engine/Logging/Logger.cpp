#include <Engine/Logging/Logger.hpp>

#include <basen.hpp>
#include <uuid.h>

pico::Engine::Logger::Logger()
    : m_dumpFolderName()
{
    std::format_to(std::back_inserter(m_dumpFolderName), "PicoLogs_{:%Y-%m-%d-%H-%M-%S}", std::chrono::system_clock::now());

    m_wideDumpFolderName = shared::Util::ToUTF16(m_dumpFolderName);

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

void pico::Engine::Logger::DumpDataToFile(pico::UnicodeStringView aFileNameTag, void* aDataStart, pico::Size aDataSize)
{
    const auto time = Clock::now().time_since_epoch().count();
    const auto tagAsNarrow = shared::Util::ToUTF8(aFileNameTag);

    // ./PicoLogs_{datetime}/{current_datetime}_{tag}_{uuid}.bin
    //
    // A note: we should be using format_to here I think
    // Ugly...

    pico::UnicodeString path{};
    std::format_to(std::back_inserter(path), L"./{}/{:#016x}_{}.bin", m_wideDumpFolderName.c_str(), time,
                   aFileNameTag);

    //auto path = shared::Util::ToUTF16(
    //    std::format("./{}/{:%Y%m%d%H%M%S}_{}_{}.bin", m_dumpFolderName.c_str(), time, aFileNameTag, uuid));

    auto [rawFile, rawFileError] = wil::try_create_new_file(path.c_str());

    if (!rawFile)
    {
        m_logger->error("Failed to open raw file for {} for write! Error {}", tagAsNarrow, rawFileError);
        return;
    }

    pico::Uint32 numberOfBytesWrittenRaw{};

    if (!WriteFile(rawFile.get(), aDataStart, (pico::Uint32)(aDataSize), (LPDWORD)(&numberOfBytesWrittenRaw),
                   nullptr) ||
        numberOfBytesWrittenRaw != (pico::Uint32)(aDataSize))
    {
        m_logger->error("Failed to write bytes to raw file for {}! Error {}", tagAsNarrow,
                        GetLastError());
        return;
    }

    m_logger->info("[Logger] Wrote {} bytes for raw {}", numberOfBytesWrittenRaw, tagAsNarrow);

    pico::UnicodeString base64Path{};
    std::format_to(std::back_inserter(base64Path), L"./{}/{:#016x}_{}_B64.txt", m_wideDumpFolderName.c_str(), time,
                   aFileNameTag);

    auto [base64File, base64FileError] = wil::try_create_new_file(base64Path.c_str());

    if (!base64File)
    {
        m_logger->error("Failed to open Base64 file for {} for write! Error {}", tagAsNarrow,
                        base64FileError);
        return;
    }

    pico::String base64{};
    bn::encode_b64((pico::Uint8*)(aDataStart), (pico::Uint8*)(aDataStart) + aDataSize, std::back_inserter(base64));

    pico::Uint32 numberOfBytesWrittenB64{};

    if (!WriteFile(base64File.get(), base64.data(), (pico::Uint32)(base64.size()), (LPDWORD)(&numberOfBytesWrittenB64),
                   nullptr) ||
        numberOfBytesWrittenB64 != (pico::Uint32)(base64.size()))
    {
        m_logger->error("Failed to write bytes to Base64 file for {}! Error {}", tagAsNarrow,
                        GetLastError());
        return;
    }

    m_logger->info("[Logger] Wrote {} bytes for Base64 {}", numberOfBytesWrittenB64, tagAsNarrow);
}

void pico::Engine::Logger::Tick()
{
    m_logger->flush();
}

pico::Engine::Logger& pico::Engine::Logger::Get()
{
    static Logger s_instance{};

    return s_instance;
}

std::shared_ptr<spdlog::logger>& pico::Engine::Logger::GetLogSink()
{
    static auto& s_logger = Get();

    return s_logger.m_logger;
}