// sensor driver for component 1 to read data from sensors by calling generateData() on the sensor type got from the config yaml of the connected sensors and creating records to be stored and sent to component 3
#pragma once
#include <chrono>
#include <cstdint>
#include <vector>
#include "common/record.h"
#include "common/data_generator.h"




class SensorDriver {
    DataGenerator* dataGenerator; // Pointer to the data generator for simulating sensor data
    std::vector<Record> records; // Vector to store the generated records
    public:
    SensorDriver(DataGenerator* dg) : dataGenerator(dg) {} // Constructor to initialize the sensor driver with a data generator
    std::vector<Record> readSensorData(); // Function to read sensor data and create records
    //function to send records to component 3 every second
};

