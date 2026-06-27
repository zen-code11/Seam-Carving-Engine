#ifndef SEAM_SOLVER_HPP
#define SEAM_SOLVER_HPP

#include "Common.hpp"

namespace SeamSolver {
    // Uses Dynamic Programming to calculate the cumulative cost and backtracks the minimum path
    std::vector<int> findVerticalSeam(const EnergyGrid& energy);

    // Carves out the computed seam from the image grid, shrinking its width by exactly 1 pixel
    ImageGrid removeVerticalSeam(const ImageGrid& image, const std::vector<int>& seam);
}

#endif // SEAM_SOLVER_HPP