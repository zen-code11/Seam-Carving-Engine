#ifndef COMMON_HPP
#define COMMON_HPP

#include <vector>
#include <string>
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
//  Core Data Types
// ─────────────────────────────────────────────────────────────────────────────

// Compact 24-bit RGB structural representation of a single image pixel.
struct Pixel {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Configuration Enumerations
// ─────────────────────────────────────────────────────────────────────────────

// Axis along which seams are computed and removed.
enum class SeamDirection {
    VERTICAL,   // Removes a column-spanning seam → shrinks image width
    HORIZONTAL  // Removes a row-spanning seam    → shrinks image height
};

// Selects the pixel energy function used to build the gradient saliency map.
enum class EnergyFunction {
    DUAL_GRADIENT,  // Dual-axis Sobel central differences across all RGB channels (default)
    LAPLACIAN       // Discrete 5-point Laplacian of the luminance channel
};

// ─────────────────────────────────────────────────────────────────────────────
//  Runtime Configuration Bundle
// ─────────────────────────────────────────────────────────────────────────────

// Encapsulates all user-configurable parameters for a single carving pipeline run.
struct CarveConfig {
    int            verticalSeams   = 0;                        // Number of vertical seams to remove
    int            horizontalSeams = 0;                        // Number of horizontal seams to remove
    EnergyFunction energyFn        = EnergyFunction::DUAL_GRADIENT;
    bool           visualize       = false;                    // Save a seam overlay image
    std::string    visualizePath;                              // Path for the overlay image
    bool           benchmark       = false;                    // Emit detailed timing metrics
};

// ─────────────────────────────────────────────────────────────────────────────
//  Matrix Type Aliases
// ─────────────────────────────────────────────────────────────────────────────

using ImageGrid  = std::vector<std::vector<Pixel>>;   // H × W pixel matrix
using EnergyGrid = std::vector<std::vector<double>>;  // H × W energy values
using CostGrid   = std::vector<std::vector<double>>;  // H × W DP cumulative cost values

#endif // COMMON_HPP