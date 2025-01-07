#pragma once
#include <Shared/Pico.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace pico::Engine
{
/**
 * \brief A data logger for development purposes. In production, all anomaly-related
 * information should be sent to the backend server. Contains functionality to log data to
 * OutputDebugString + log file sinks, as well as dump files to the disk.
 */
struct Logger : shared::Util::NonCopyableOrMovable
{
    std::shared_ptr<spdlog::logger> m_logger{};
    std::shared_ptr<spdlog::sinks::msvc_sink_mt> m_debugStringSink{};
    std::shared_ptr<spdlog::sinks::basic_file_sink_mt> m_fileSink{};

    // Name of the folder we will output the logs/dumped data to.
    pico::String m_dumpFolderName{};

    Logger() noexcept;

    /**
     * \brief Dumps arbitrary binary data to a file. Binary data will be Base64-encoded.
     * 
     * \param aFileNameTag An identifier tag that will be incorporated into the file name.
     * \param aDataStart Pointer to the start of the data you want to dump.
     * \param aDataSize Size of the data pointed to by aDataStart.
     */
    void DumpDataToFile(pico::StringView aFileNameTag, void* aDataStart, pico::Size aDataSize) noexcept;

    /**
     * \brief Get a singleton instance of the logger.
     * \return A singleton instance of the logger.
     */
    static Logger& Get() noexcept;

    /**
     * \brief Get an instance of the spdlog logger - useful to not type out m_logger constantly
     * 
     * \return An instance of the spdlog logger.
     */
    static std::shared_ptr<spdlog::logger>& GetLogSink() noexcept;
};
}