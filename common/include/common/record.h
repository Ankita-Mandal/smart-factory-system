// Record structure for storing data containing a timestamp and an array of double data values.
#pragma once
#include <chrono>
#include <cstdint>
#include <vector>

struct Record {
    std::uint64_t sequence{};
    std::chrono::system_clock::time_point timestamp; // Timestamp of the record
    std::vector<double> data; // Array of double data values
};

