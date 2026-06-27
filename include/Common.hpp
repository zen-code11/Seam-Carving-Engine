#ifndef COMMON_HPP
#define COMMON_HPP

#include <vector>
#include <cstdint>

// Compact 24-bit RGB structural representation of a single pixel
struct Pixel
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

// Type aliases for cleaner matrix handling throughout the pipeline
using ImageGrid = std::vector<std::vector<Pixel>>;
using EnergyGrid = std::vector<std::vector<double>>;
using CostGrid = std::vector<std::vector<double>>;

#endif // COMMON_HPP