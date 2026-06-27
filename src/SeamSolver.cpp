#include "SeamSolver.hpp"
#include <algorithm>
#include <limits>

namespace SeamSolver {

    std::vector<int> findVerticalSeam(const EnergyGrid& energy) {
        int H = energy.size();
        int W = energy[0].size();

        // Step 1: Instantiate and initialize the DP Cumulative Cost Matrix
        CostGrid cost(H, std::vector<double>(W, 0.0));
        
        // Base case: The top row's cumulative cost is just its initial pixel energy
        for (int j = 0; j < W; ++j) {
            cost[0][j] = energy[0][j];
        }

        // Step 2: Build the DP table row-by-row
        for (int i = 1; i < H; ++i) {
            for (int j = 0; j < W; ++j) {
                // Safeguard boundaries for left, center, and right parents in the row above
                double left   = (j > 0) ? cost[i - 1][j - 1] : std::numeric_limits<double>::max();
                double center = cost[i - 1][j];
                double right  = (j < W - 1) ? cost[i - 1][j + 1] : std::numeric_limits<double>::max();

                // Recurrence Relation: current energy + min of the three valid parent pathways
                cost[i][j] = energy[i][j] + std::min({left, center, right});
            }
        }

        // Step 3: Backtrack to extract the optimal minimum-energy path
        std::vector<int> seam(H);

        // Find the absolute lowest score in the final row to anchor our seam base
        int minIdx = 0;
        double minCost = cost[H - 1][0];
        for (int j = 1; j < W; ++j) {
            if (cost[H - 1][j] < minCost) {
                minCost = cost[H - 1][j];
                minIdx = j;
            }
        }
        seam[H - 1] = minIdx;

        // Trace the path backwards up to the top row (row 0)
        for (int i = H - 2; i >= 0; --i) {
            int currCol = seam[i + 1];

            int nextCol = currCol;
            double nextCost = cost[i][currCol];

            // Evaluate left parent
            if (currCol > 0 && cost[i][currCol - 1] < nextCost) {
                nextCost = cost[i][currCol - 1];
                nextCol = currCol - 1;
            }
            // Evaluate right parent
            if (currCol < W - 1 && cost[i][currCol + 1] < nextCost) {
                nextCol = currCol + 1;
            }

            seam[i] = nextCol;
        }

        return seam;
    }

    ImageGrid removeVerticalSeam(const ImageGrid& image, const std::vector<int>& seam) {
        int H = image.size();
        int W = image[0].size();

        // The target grid will have the exact same height, but its width is reduced by 1
        ImageGrid mutatedImage(H, std::vector<Pixel>(W - 1));

        for (int i = 0; i < H; ++i) {
            int seamCol = seam[i];
            int targetCol = 0;

            for (int j = 0; j < W; ++j) {
                // Skip the exact pixel coordinate mapped by our backtracking seam
                if (j == seamCol) continue;

                // Copy over all surviving pixels linearly
                mutatedImage[i][targetCol] = image[i][j];
                targetCol++;
            }
        }

        return mutatedImage;
    }
}