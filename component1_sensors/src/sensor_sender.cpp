#include "sensor_sender.h"

#include <cerrno>
#include <cstring>
#include <utility>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace {


nlohmann::json batchToJson(const SensorBatch& batch) {
    nlohmann::json sensors = nlohmann::json::array();

    for (const SensorRecord& sensor : batch.sensors) {
        sensors.push_back({
            {"id", sensor.sensorId},
            {"sequence", sensor.record.sequence},
            {"timestamp_ms", batch.timestampMs},
            {"data", sensor.record.data}
        });
    }

    return {
        {"sequence", batch.sequence},
        {"timestamp_ms", batch.timestampMs},
        {"sensors", sensors}
    };
}

}

SensorSender::SensorSender(std::string socketPath)
    : socketPath_(std::move(socketPath)) {
}

SensorSender::~SensorSender() {
    disconnect();
}

bool SensorSender::connect() {
    if (socketFd_ >= 0) {
        return true;
    }

    socketFd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);

    if (socketFd_ < 0) {
        spdlog::error("Could not create sensor socket: {}",
                      std::strerror(errno));
        return false;
    }

    sockaddr_un address{};
    address.sun_family = AF_UNIX;

    if (socketPath_.size() >= sizeof(address.sun_path)) {
        spdlog::error("Sensor socket path is too long");
        disconnect();
        return false;
    }

    std::strncpy(
        address.sun_path,
        socketPath_.c_str(),
        sizeof(address.sun_path) - 1
    );

    if (::connect(
            socketFd_,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)
        ) < 0) {
        spdlog::debug("Component 3 is unavailable: {}",
                      std::strerror(errno));
        disconnect();
        return false;
    }

    spdlog::info("Sensor sender connected to {}", socketPath_);
    return true;
}

bool SensorSender::send(const SensorBatch& batch) {
    if (socketFd_ < 0 && !connect()) {
        return false;
    }

    const std::string payload =
        batchToJson(batch).dump() + '\n';

    if (!writeAll(payload)) {
        spdlog::warn(
            "Failed to send sensor batch sequence={}",
            batch.sequence
        );
        disconnect();
        return false;
    }

    spdlog::debug(
        "Sent sensor batch sequence={} sensor_count={}",
        batch.sequence,
        batch.sensors.size()
    );

    return true;
}

bool SensorSender::writeAll(const std::string& payload) {
    std::size_t written = 0;

    while (written < payload.size()) {
        const ssize_t result = ::write(
            socketFd_,
            payload.data() + written,
            payload.size() - written
        );

        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }

            spdlog::error("Could not write sensor batch: {}",
                          std::strerror(errno));
            return false;
        }

        if (result == 0) {
            return false;
        }

        written += static_cast<std::size_t>(result);
    }

    return true;
}

void SensorSender::disconnect() {
    if (socketFd_ >= 0) {
        ::close(socketFd_);
        socketFd_ = -1;
        spdlog::info("Sensor sender disconnected");
    }
}