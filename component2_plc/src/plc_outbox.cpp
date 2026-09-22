
#include "plc_outbox.h"

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

nlohmann::json recordToJson(const Record& record) {
    return {
        {"sequence", record.sequence},
        {"timestamp_ms", timestampToMilliseconds(record.timestamp)},
        {"data", record.data}
    };
}

Record recordFromJson(const nlohmann::json& json) {
    Record record;
    record.sequence = json.at("sequence").get<std::uint64_t>();
    record.timestamp = millisecondsToTimestamp(
        json.at("timestamp_ms").get<std::int64_t>()
    );
    record.data = json.at("data").get<std::vector<double>>();
    return record;
}

}

PLCOutbox::PLCOutbox(std::string dataPath) : dataPath_(std::move(dataPath)), cursorPath_(dataPath_+".cursor") {
    loadState();
}

std::uint64_t PLCOutbox::append(Record record) {
    
    record.sequence = nextSequence_++;
    std::ofstream output(dataPath_, std::ios::app);
    if (!output) {
        throw std::runtime_error("Could not open outbox: " + dataPath_);
    }

    output << recordToJson(record).dump() << '\n';
    output.flush();

    if (!output) {
        throw std::runtime_error("Could not write outbox: " + dataPath_);
    }

    return record.sequence;
}

std::vector<Record> PLCOutbox::pending() const {
    std::vector<Record> records;
    std::ifstream input(dataPath_);
    if (!input) {
        return records;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        Record record = recordFromJson(nlohmann::json::parse(line));

        if (record.sequence > acknowledgedSequence_) {
            records.push_back(std::move(record));
        }
    }
    return records;
}

void PLCOutbox::acknowledge(std::uint64_t sequence) {
    if(sequence<= acknowledgedSequence_){
        return;
    }
    acknowledgedSequence_ = sequence;
    writeCursor();
}

void PLCOutbox::loadState() {
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

void PLCOutbox::writeCursor() const {
    std::ofstream cursor(cursorPath_, std::ios::trunc);

    if (!cursor) {
        throw std::runtime_error("Could not open cursor: " + cursorPath_);
    }

    cursor << acknowledgedSequence_ << '\n';
    cursor.flush();
}
