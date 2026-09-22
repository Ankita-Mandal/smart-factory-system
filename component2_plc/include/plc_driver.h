
#pragma once
#include "common/record.h"

class PLCDriver {
    public:
    PLCDriver() = default;
    Record readPLCRecord() const; // Function to read PLC data and create records
};