#include "common/logger.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace {
    


constexpr const char* SensorSocketPath = "/tmp/component1_socket";
constexpr const char* PlcSocketPath = "/tmp/component2_socket";

using Json = nlohmann::json;

struct PendingCombination {
    Json sensorBatch;
    Json plcRecord;
    bool hasSensor{false};
    bool hasPlc{false};
};

std::int64_t timestampBucket(std::int64_t timestampMs) {
    return timestampMs / 1000; 
}

int createListeningSocket(const char* path) {
    ::unlink(path);

    const int socketFd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (socketFd < 0) {
        perror("socket");
        return -1;
    }

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    std::strncpy(
        address.sun_path,
        path,
        sizeof(address.sun_path) - 1
    );

    if (::bind(
            socketFd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)
        ) < 0) {
        perror("bind");
        ::close(socketFd);
        return -1;
    }

    if (::listen(socketFd, 1) < 0) {
        perror("listen");
        ::close(socketFd);
        return -1;
    }

    return socketFd;
}

int acceptConnection(int listeningFd) {
    return ::accept(listeningFd, nullptr, nullptr);
}

bool sendAcknowledgement(int clientFd, std::uint64_t sequence) {
    const std::string acknowledgement =
        Json{{"ack", sequence}}.dump() + '\n';

    return ::write(
        clientFd,
        acknowledgement.data(),
        acknowledgement.size()
    ) == static_cast<ssize_t>(acknowledgement.size());
}

void combineIfReady( std::unordered_map<std::int64_t, PendingCombination>& pending, std::int64_t bucket, std::ofstream& output) 
{
    auto iterator = pending.find(bucket);

    if (iterator == pending.end()) {
        spdlog::warn(
            "Data missing at time:",
            bucket
        );
        return;

    }

    PendingCombination& combination = iterator->second;

    if (!combination.hasSensor || !combination.hasPlc) {
        return;
    }

    Json combined{
        {"timestamp_bucket", bucket},
        {"sensors", combination.sensorBatch.at("sensors")},
        {"plc", combination.plcRecord}
    };

    output << combined.dump() << '\n';
    output.flush();

    pending.erase(iterator);
    spdlog::info(
        "[component3] wrote combined record for bucket ",
        bucket
    );
}

void processLine(
    const std::string& line,
    bool fromSensor,
    int clientFd,
    std::unordered_map<std::int64_t, PendingCombination>& pending,
    std::ofstream& output
) {
    if (line.empty()) {
        return;
    }

    try {
        const Json message = Json::parse(line);
        const auto timestampMs =
            message.at("timestamp_ms").get<std::int64_t>();
        const auto sequence =
            message.at("sequence").get<std::uint64_t>();

        const auto bucket = timestampBucket(timestampMs);
        auto& combination = pending[bucket];

        if (fromSensor) {
            combination.sensorBatch = message;
            combination.hasSensor = true;
        } else {
            combination.plcRecord = message;
            combination.hasPlc = true;
        }

        if (!sendAcknowledgement(clientFd, sequence)) {
            spdlog::warn(
                "[component3] failed to send acknowledgement\n"
            );
        }

        combineIfReady(pending, bucket, output);
    } catch (const std::exception& error) {
        spdlog::warn(
            "[component3] invalid JSON: ",
            error.what()
        );
    }
}

}

int main() {
    sfs::initializeLogging("component3");
    const int sensorListeningFd =
        createListeningSocket(SensorSocketPath);
    const int plcListeningFd =
        createListeningSocket(PlcSocketPath);
    if (sensorListeningFd < 0 || plcListeningFd < 0) {
        return 1;
    }
    spdlog::info(
        "[component3] waiting for component connections\n"
    );

    const int sensorClientFd = acceptConnection(sensorListeningFd);
    const int plcClientFd = acceptConnection(plcListeningFd);

    if (sensorClientFd < 0 || plcClientFd < 0) {
        return 1;
    }

    std::ofstream output("component3_combine/combination.jsonl", std::ios::app);

    if (!output) {
        spdlog::warn(
            "[component3] could not open combination.jsonl\n"
        );
    }

    std::unordered_map<std::int64_t, PendingCombination> pending;
    std::string sensorBuffer;
    std::string plcBuffer;

    pollfd sockets[2]{
        {sensorClientFd, POLLIN, 0},
        {plcClientFd, POLLIN, 0}
    };

    char buffer[4096];

    while (true) {
        const int result = ::poll(sockets, 2, -1);

        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("poll");
            break;
        }

        for (int index = 0; index < 2; ++index) {
            if (!(sockets[index].revents & POLLIN)) {
                continue;
            }

            const ssize_t bytesRead =
                ::read(sockets[index].fd, buffer, sizeof(buffer));

            if (bytesRead <= 0) {
                sockets[index].fd = -1;
                continue;
            }

            std::string& messageBuffer =
                index == 0 ? sensorBuffer : plcBuffer;

            messageBuffer.append(buffer, bytesRead);

            std::size_t newlinePosition;

            while ((newlinePosition =
                        messageBuffer.find('\n')) != std::string::npos) {
                const std::string line =
                    messageBuffer.substr(0, newlinePosition);

                messageBuffer.erase(0, newlinePosition + 1);

                processLine(
                    line,
                    index == 0,
                    sockets[index].fd,
                    pending,
                    output
                );
            }
        }
    }

    ::close(sensorClientFd);
    ::close(plcClientFd);
    ::close(sensorListeningFd);
    ::close(plcListeningFd);

    return 0;
}