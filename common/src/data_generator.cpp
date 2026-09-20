// simulate data vectors for sensor and plc data with random values when called by component 1 or component 2

#include "common/data_generator.h"
#include <vector>
#include <random>

std::vector<double> DataGenerator::generateData() {
    std::vector<double> data(size); //vector to hold the generated data
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(min, max);

    for (size_t i = 0; i < size; ++i) {
        data[i] = distribution(generator);
    }

    return data;
}