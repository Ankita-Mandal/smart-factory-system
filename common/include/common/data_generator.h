// Data generator for simulating sensor and plc data
#pragma once
#include <random>
#include <vector>

class DataGenerator {
    size_t size; // Size of the data array
    double min; // Minimum value for the random data
    double max; // Maximum value for the random data
    public:
    DataGenerator(size_t s, double mn, double mx) : size(s), min(mn), max(mx) {};

    std::vector<double> generateData(); 
};
