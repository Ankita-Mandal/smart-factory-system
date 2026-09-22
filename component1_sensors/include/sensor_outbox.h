//store recorded sensor data for all n sensors in jsonl and track what was the last ack data
#pragma once
#include "common/record.h"
#include <vector>
#include <cstdint>
#include <string>

struct SensorRecord{
    std::string sensorId;
    Record record;
};

struct SensorBatch {
    std::uint64_t sequence{};
    std::int64_t timestampMs{};
    std::vector<SensorRecord> sensors;
};

class SensorOutbox{
    public:
        explicit SensorOutbox(std::string dataPath);

        std::uint64_t append(SensorBatch batch);
        std::vector<SensorBatch> pending() const;
        void acknowledge(std::uint64_t sequence);

    private:
        std::string dataPath_;
        std::string cursorPath_;
        std::uint64_t nextSequence_{1};
        std::uint64_t acknowledgedSequence_{};

        void loadState();
        void writeCursor() const;    
};