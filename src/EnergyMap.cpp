#include "EnergyMap.hpp"
#include <cmath>

namespace EnergyMap {

    EnergyGrid calculateEnergy(const ImageGrid& image) {
        int H = image.size();
        int W = image[0].size();
        
        // Instantiate our energy surface matrix matching the original dimensions
        EnergyGrid energy(H, std::vector<double>(W, 0.0));

        for (int i = 0; i < H; ++i) {
            for (int j = 0; j < W; ++j) {
                
                // --- Horizontal (X) Boundary Wrap-Around Logic ---
                int leftX  = (j == 0) ? W - 1 : j - 1;
                int rightX = (j == W - 1) ? 0 : j + 1;

                // --- Vertical (Y) Boundary Wrap-Around Logic ---
                int topY    = (i == 0) ? H - 1 : i - 1;
                int bottomY = (i == H - 1) ? 0 : i + 1;

                // Compute horizontal color channel differentials
                double rx = image[i][rightX].r - image[i][leftX].r;
                double gx = image[i][rightX].g - image[i][leftX].g;
                double bx = image[i][rightX].b - image[i][leftX].b;
                double deltaXSq = (rx * rx) + (gx * gx) + (bx * bx); //  Fixed typo here

                // Compute vertical color channel differentials
                double ry = image[bottomY][j].r - image[topY][j].r;
                double gy = image[bottomY][j].g - image[topY][j].g;
                double by = image[bottomY][j].b - image[topY][j].b;
                double deltaYSq = (ry * ry) + (gy * gy) + (by * by);

                // Total energy score is the Euclidean norm of both gradients
                energy[i][j] = std::sqrt(deltaXSq + deltaYSq);
            }
        }

        return energy;
    }
}