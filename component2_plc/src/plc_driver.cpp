// Implementation of the PLCDriver class to read data records 

#include "plc_driver.h"

#include <random>
#include <chrono>

Record PLCDriver::readPLCRecord() const {
    // Simulate reading data from a PLC and generating a record
    static std::mt19937 gen{std::random_device{}()};
    static std::uniform_real_distribution<double> dist(0.0, 20.0);

    Record record;
    // Set the timestamp to the current time
    record.timestamp = std::chrono::system_clock::now();
    record.data.resize(5);
    for (double& v : record.data) {
        v = dist(gen);
    }
    return record;
}
