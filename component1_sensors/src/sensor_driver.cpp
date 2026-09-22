// Sensor driver implementation for component 1. 
#include "sensor_driver.h"
#include <chrono>
#include <random>

Record SensorDriver::readSensorData() const{
    //TODO: take sensor type input and generate separate types of data
    static std::mt19937 gen{std::random_device{}()};
    static std::uniform_real_distribution<double> dist(30.0, 70.0);

    Record record;
    record.timestamp = std::chrono::system_clock::now();
    record.data.resize(7);
    for (double& v : record.data) {
        v = dist(gen);
    }

    return record;
}
