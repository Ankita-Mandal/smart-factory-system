#include "plc_sender.h"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <string>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace {

std::int64_t timestampToMilliseconds(
    const std::chrono::system_clock::time_point& timestamp
) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()
    ).count();
}

}

PLCSender::PLCSender(std::string socketPath)
    : socketPath_(std::move(socketPath)) {
}

PLCSender::~PLCSender() {
    disconnect();
}

bool PLCSender::connect() {
    if (socketFd_ >= 0) {
        return true;
    }

    socketFd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);

    if (socketFd_ < 0) {
        spdlog::error("socket() failed: {}", std::strerror(errno));
        return false;
    }

    sockaddr_un address{};
    address.sun_family = AF_UNIX;

    if (socketPath_.size() >= sizeof(address.sun_path)) {
        spdlog::error("Socket path is too long: {}", socketPath_);
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
        spdlog::debug(
            "Could not connect to component3: {}",
            std::strerror(errno)
        );
        disconnect();
        return false;
    }

    spdlog::info("Connected to component3 at {}", socketPath_);
    return true;
}

bool PLCSender::send(const Record& record) {
    if (socketFd_ < 0 && !connect()) {
        return false;
    }

    nlohmann::json message{
        {"sequence", record.sequence},
        {"timestamp_ms", timestampToMilliseconds(record.timestamp)},
        {"data", record.data}
    };

    // Newline-delimited JSON provides message framing.
    const std::string payload = message.dump() + '\n';

    if (!writeAll(payload)) {
        spdlog::warn(
            "Failed to send PLC record sequence={}",
            record.sequence
        );
        disconnect();
        return false;
    }

    spdlog::debug(
        "Sent PLC record sequence={} values={}",
        record.sequence,
        record.data.size()
    );

    return true;
}

bool PLCSender::writeAll(const std::string& payload) {
    std::size_t bytesWritten = 0;

    while (bytesWritten < payload.size()) {
        const ssize_t result = ::write(
            socketFd_,
            payload.data() + bytesWritten,
            payload.size() - bytesWritten
        );

        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }

            spdlog::error("write() failed: {}", std::strerror(errno));
            return false;
        }

        if (result == 0) {
            return false;
        }

        bytesWritten += static_cast<std::size_t>(result);
    }

    return true;
}

void PLCSender::disconnect() {
    if (socketFd_ >= 0) {
        ::close(socketFd_);
        socketFd_ = -1;
        spdlog::info("Disconnected from component3");
    }
}