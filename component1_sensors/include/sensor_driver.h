// sensor driver for component 1 to read data from sensors by calling generateData() on the sensor type got from the config yaml of the connected sensors and creating records to be stored and sent to component 3
#pragma once

#include "common/record.h"


class SensorDriver {
    public:
    SensorDriver() = default;
    Record readSensorData() const; 
};

