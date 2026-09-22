
#include "common/logger.h"
#include "sensor_driver.h"
#include "sensor_outbox.h"
#include "sensor_sender.h"

// #include <yaml-cpp/yaml.h>
#include <chrono>
#include <thread>
#include <memory>
#include <cstdint>
#include <string>
#include <vector>
#include <utility>
#include <spdlog/spdlog.h>
#include <csignal>

namespace {

struct SensorConfig {
    std::string id;
    char type;
};

std::int64_t currentTimestampMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

}

int main(){
    std::signal(SIGPIPE, SIG_IGN);
    sfs::initializeLogging("component1");
    //TODO: implement reading from config.yaml

    const std::vector<SensorConfig> sensors{
        {"sensor_1", 'A'},
        {"sensor_2", 'B'},
        {"sensor_3", 'A'}
    };

    SensorDriver sensorDriver;
    SensorOutbox outbox("component1_sensors/sensor_data.jsonl");
    SensorSender sender("/tmp/component1_socket");

    spdlog::info( "Component 1 started with {} sensors", sensors.size()
    );

    while (true) {
        const auto nextReadTime =
            std::chrono::steady_clock::now() +
            std::chrono::seconds(1);

        SensorBatch batch;
        batch.timestampMs = currentTimestampMs();

        for (const SensorConfig& config : sensors) {
            SensorRecord sensorRecord;
            sensorRecord.sensorId = config.id;
            sensorRecord.record = sensorDriver.readSensorData();

            // All sensors in this batch share one correlation timestamp.
            sensorRecord.record.timestamp =
                std::chrono::system_clock::time_point{
                    std::chrono::milliseconds(batch.timestampMs)
                };

            batch.sensors.push_back(std::move(sensorRecord));
        }

        const std::uint64_t batchSequence = outbox.append(std::move(batch));

        spdlog::debug(
            "Stored sensor batch sequence={} sensor_count={}",
            batchSequence,
            sensors.size()
        );

        // If component 3 is offline, send() returns false and
        // the records remain in the outbox.
        for (const SensorBatch& pendingBatch : outbox.pending()) {
            if (!sender.send(pendingBatch)) {
                spdlog::debug(
                    "Component 3 unavailable; pending batches remain queued"
                );
                break;
            }

            outbox.acknowledge(pendingBatch.sequence);

            spdlog::debug(
                "Acknowledged sensor batch sequence={}",
                pendingBatch.sequence
            );
        }

        std::this_thread::sleep_until(nextReadTime);
    }
}