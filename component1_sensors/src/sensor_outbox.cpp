
#include "sensor_outbox.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {

std::int64_t timestampToMilliseconds(
    const std::chrono::system_clock::time_point& timestamp
) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()
    ).count();
}

std::chrono::system_clock::time_point millisecondsToTimestamp(
    std::int64_t timestamp
) {
    return std::chrono::system_clock::time_point{
        std::chrono::milliseconds{timestamp}
    };
}

nlohmann::json recordToJson(
    const SensorRecord& sensorRecord,
    std::int64_t batchTimestamp
) {
    const Record& record = sensorRecord.record;

    return {
        {"id", sensorRecord.sensorId},
        {"sequence", record.sequence},
        {"timestamp_ms", batchTimestamp},
        {"data", record.data}
    };
}

SensorRecord recordFromJson(const nlohmann::json& json) {
    SensorRecord sensorRecord;

    sensorRecord.sensorId = json.at("id").get<std::string>();
    sensorRecord.record.sequence =
        json.at("sequence").get<std::uint64_t>();

    sensorRecord.record.timestamp = millisecondsToTimestamp(
        json.at("timestamp_ms").get<std::int64_t>()
    );

    sensorRecord.record.data =
        json.at("data").get<std::vector<double>>();

    return sensorRecord;
}
nlohmann::json batchToJson(const SensorBatch& batch) {
    nlohmann::json sensors = nlohmann::json::array();

    for (const SensorRecord& sensor : batch.sensors) {
        sensors.push_back(recordToJson(sensor, batch.timestampMs));
    }

    return {
        {"sequence", batch.sequence},
        {"timestamp_ms", batch.timestampMs},
        {"sensors", sensors}
    };
}
SensorBatch batchFromJson(const nlohmann::json& json) {
    SensorBatch batch;

    batch.sequence = json.at("sequence").get<std::uint64_t>();
    batch.timestampMs = json.at("timestamp_ms").get<std::int64_t>();

    for (const auto& sensorJson : json.at("sensors")) {
        batch.sensors.push_back(recordFromJson(sensorJson));
    }

    return batch;
}

}

SensorOutbox::SensorOutbox(std::string dataPath)
    : dataPath_(std::move(dataPath)),
      cursorPath_(dataPath_ + ".cursor") {
    loadState();
}

std::uint64_t SensorOutbox::append(SensorBatch batch) {
    batch.sequence = nextSequence_++;

    for (SensorRecord& sensor : batch.sensors) {
        sensor.record.sequence = batch.sequence;
        sensor.record.timestamp = std::chrono::system_clock::time_point{
            std::chrono::milliseconds(batch.timestampMs)
        };
    }

    std::ofstream output(dataPath_, std::ios::app);

    if (!output) {
        throw std::runtime_error(
            "Could not open sensor outbox: " + dataPath_
        );
    }

    output << batchToJson(batch).dump() << '\n';
    output.flush();

    if (!output) {
        throw std::runtime_error(
            "Could not write sensor outbox: " + dataPath_
        );
    }

    return batch.sequence;
}

std::vector<SensorBatch> SensorOutbox::pending() const {
    std::vector<SensorBatch> batches;
    std::ifstream input(dataPath_);

    if (!input) {
        return batches;
    }

    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        SensorBatch batch = batchFromJson(
            nlohmann::json::parse(line)
        );

        if (batch.sequence > acknowledgedSequence_) {
            batches.push_back(std::move(batch));
        }
    }

    return batches;
}

void SensorOutbox::acknowledge(std::uint64_t sequence) {
    if (sequence <= acknowledgedSequence_) {
        return;
    }

    acknowledgedSequence_ = sequence;
    writeCursor();
}

void SensorOutbox::loadState() {
    std::ifstream cursor(cursorPath_);

    if (cursor) {
        cursor >> acknowledgedSequence_;
    }

    std::ifstream input(dataPath_);
    std::string line;
    std::uint64_t largestSequence = acknowledgedSequence_;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        const auto json = nlohmann::json::parse(line);

        largestSequence = std::max(
            largestSequence,
            json.at("sequence").get<std::uint64_t>()
        );
    }

    nextSequence_ = largestSequence + 1;
}

void SensorOutbox::writeCursor() const {
    const std::string temporaryPath = cursorPath_ + ".tmp";

    std::ofstream output(temporaryPath, std::ios::trunc);

    if (!output) {
        throw std::runtime_error(
            "Could not open cursor: " + temporaryPath
        );
    }

    output << acknowledgedSequence_ << '\n';
    output.flush();
    output.close();

    if (std::rename(temporaryPath.c_str(), cursorPath_.c_str()) != 0) {
        throw std::runtime_error(
            "Could not replace cursor: " + cursorPath_
        );
    }
}
