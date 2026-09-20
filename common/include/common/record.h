// Record structure for storing data containing a timestamp and an array of double data values.
#pragma once
#include <chrono>
#include <cstdint>
#include <vector>

struct Record {
    std::chrono::system_clock::time_point timestamp; // Timestamp of the record
    std::vector<double> data; // Array of double data values

    // Constructor to initialize the record with a timestamp and data values
    Record(const std::chrono::system_clock::time_point& ts, const std::vector<double>& d)
        : timestamp(ts), data(d) {}

};

class RecordGenerator {
    //creates a record with the current timestamp and a vector of double data values
    public:
    static Record generateRecord(std::chrono::system_clock::time_point ts, const std::vector<double>& data);
};