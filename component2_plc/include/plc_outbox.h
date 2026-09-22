//store recorded plc data in jsonl and track what was last acknowledged
#pragma once
#include "common/record.h"
#include <cstdint>
#include <string>
#include <vector>


class PLCOutbox {
public:
    explicit PLCOutbox(std::string dataPath);

    std::uint64_t append(Record record);
    std::vector<Record> pending() const;
    void acknowledge(std::uint64_t sequence);

private:
    std::string dataPath_;
    std::string cursorPath_;
    std::uint64_t nextSequence_{};
    std::uint64_t acknowledgedSequence_{};

    void loadState();
    void writeCursor() const;
};