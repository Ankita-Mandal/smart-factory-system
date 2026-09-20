// Sensor driver implementation for component 1. 
#include "sensor_driver.h"
#include <chrono>

std::vector<Record> SensorDriver::readSensorData() {
    // call the generateData() function of the data generator to get the sensor data based on sensor type and it's size, min and max values from the config yaml of the connected sensors
    std::vector<double> sensorData = dataGenerator->generateData();
    // create a record with the current timestamp and the generated sensor data
    Record record(std::chrono::system_clock::now(), sensorData);
    // store the record in the records vector
    records.push_back(record);    
    return records;
}
