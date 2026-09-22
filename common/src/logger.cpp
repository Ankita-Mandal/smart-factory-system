#include "common/logger.h"

#include <filesystem>
#include <memory>
#include <vector>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace sfs {

void initializeLogging(const std::string& component) {

    auto consoleSink =
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    auto fileSink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            "logs/" + component + ".log",
            true
        );

    std::vector<spdlog::sink_ptr> sinks{
        consoleSink,
        fileSink
    };

    auto logger = std::make_shared<spdlog::logger>(
        component,
        sinks.begin(),
        sinks.end()
    );
    logger->flush_on(spdlog::level::info);
    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::warn);

    spdlog::set_default_logger(logger);
    spdlog::set_pattern(
        "%Y-%m-%d %H:%M:%S.%e [%^%l%$] [%n] %v"
    );
}

}