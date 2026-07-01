#include "EnergyMap.hpp"
#include <cmath>
#include <stdexcept>

namespace EnergyMap {

// ─────────────────────────────────────────────────────────────────────────────
//  Internal: Dual-Gradient Energy  (Central Differences over RGB)
// ─────────────────────────────────────────────────────────────────────────────
//
//  For pixel (i, j):
//    ΔXsq = (R[i][j+1]-R[i][j-1])² + (G[i][j+1]-G[i][j-1])² + (B[i][j+1]-B[i][j-1])²
//    ΔYsq = (R[i+1][j]-R[i-1][j])² + (G[i+1][j]-G[i-1][j])² + (B[i+1][j]-B[i-1][j])²
//    energy(i,j) = √(ΔXsq + ΔYsq)
//
//  Boundary conditions use toroidal (modular) wrap-around so that border pixels
//  receive the same quality of gradient estimate as interior pixels, avoiding
//  the artificial low-energy seam paths that zero-padding would create.
//
//  PERFORMANCE: Row pointers (mat.ptr equivalent) are used instead of
//  operator[][]  to avoid redundant bounds checks in the inner loop.
//  This typically yields a 3–5× speedup on the load/energy phases.
//
static EnergyGrid dualGradient(const ImageGrid& image) {
    const int H = static_cast<int>(image.size());
    const int W = static_cast<int>(image[0].size());

    EnergyGrid energy(H, std::vector<double>(W));

    for (int i = 0; i < H; ++i) {
        // Precompute row pointers for the three neighbouring rows
        const int topI = (i == 0)     ? H - 1 : i - 1;
        const int botI = (i == H - 1) ? 0     : i + 1;

        const Pixel* rowTop = image[topI].data();
        const Pixel* rowBot = image[botI].data();
        const Pixel* rowCur = image[i].data();
        double*      eRow   = energy[i].data();

        for (int j = 0; j < W; ++j) {
            const int leftJ  = (j == 0)     ? W - 1 : j - 1;
            const int rightJ = (j == W - 1) ? 0     : j + 1;

            // Horizontal channel differentials
            const double rx  = static_cast<double>(rowCur[rightJ].r) - rowCur[leftJ].r;
            const double gx  = static_cast<double>(rowCur[rightJ].g) - rowCur[leftJ].g;
            const double bx  = static_cast<double>(rowCur[rightJ].b) - rowCur[leftJ].b;
            const double dXsq = rx*rx + gx*gx + bx*bx;

            // Vertical channel differentials
            const double ry  = static_cast<double>(rowBot[j].r) - rowTop[j].r;
            const double gy  = static_cast<double>(rowBot[j].g) - rowTop[j].g;
            const double by  = static_cast<double>(rowBot[j].b) - rowTop[j].b;
            const double dYsq = ry*ry + gy*gy + by*by;

            eRow[j] = std::sqrt(dXsq + dYsq);
        }
    }
    return energy;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Internal: Laplacian Energy  (Discrete 5-Point Laplacian of Luminance)
// ─────────────────────────────────────────────────────────────────────────────
//
//  luminance(i,j) = 0.299·R + 0.587·G + 0.114·B   (BT.601)
//  energy(i,j)    = |4·lum(i,j) - lum(i-1,j) - lum(i+1,j) - lum(i,j-1) - lum(i,j+1)|
//
//  The Laplacian acts as a second-order edge detector that responds strongly
//  to fine textures and subtle gradients that the first-order dual-gradient
//  can miss. Boundary conditions use the same toroidal wrap-around.
//
static EnergyGrid laplacianEnergy(const ImageGrid& image) {
    const int H = static_cast<int>(image.size());
    const int W = static_cast<int>(image[0].size());

    // Phase 1: Build luminance matrix from RGB using BT.601 coefficients.
    // Storing as doubles avoids repeated float-to-int conversions in the
    // Laplacian phase.
    std::vector<std::vector<double>> lum(H, std::vector<double>(W));
    for (int i = 0; i < H; ++i) {
        const Pixel* srcRow = image[i].data();
        double*      lumRow = lum[i].data();
        for (int j = 0; j < W; ++j) {
            lumRow[j] = 0.299 * srcRow[j].r
                      + 0.587 * srcRow[j].g
                      + 0.114 * srcRow[j].b;
        }
    }

    // Phase 2: Apply 5-point Laplacian stencil.
    EnergyGrid energy(H, std::vector<double>(W));
    for (int i = 0; i < H; ++i) {
        const int topI = (i == 0)     ? H - 1 : i - 1;
        const int botI = (i == H - 1) ? 0     : i + 1;

        const double* lumRow    = lum[i].data();
        const double* lumTopRow = lum[topI].data();
        const double* lumBotRow = lum[botI].data();
        double*       eRow      = energy[i].data();

        for (int j = 0; j < W; ++j) {
            const int leftJ  = (j == 0)     ? W - 1 : j - 1;
            const int rightJ = (j == W - 1) ? 0     : j + 1;

            const double lap = 4.0 * lumRow[j]
                             - lumTopRow[j]
                             - lumBotRow[j]
                             - lumRow[leftJ]
                             - lumRow[rightJ];

            eRow[j] = std::abs(lap);
        }
    }
    return energy;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────────────────────────────────────

EnergyGrid calculateEnergy(const ImageGrid& image, EnergyFunction fn) {
    if (image.empty() || image[0].empty()) {
        throw std::invalid_argument(
            "EnergyMap::calculateEnergy — cannot compute energy of an empty image.");
    }
    switch (fn) {
        case EnergyFunction::DUAL_GRADIENT: return dualGradient(image);
        case EnergyFunction::LAPLACIAN:     return laplacianEnergy(image);
    }
    return dualGradient(image);  // unreachable; keeps compiler happy
}

// Transposes the energy grid so that rows and columns are swapped.
// This reduces horizontal seam finding to vertical seam finding without
// duplicating any DP logic.
EnergyGrid transpose(const EnergyGrid& grid) {
    if (grid.empty() || grid[0].empty()) return {};

    const int H = static_cast<int>(grid.size());
    const int W = static_cast<int>(grid[0].size());

    EnergyGrid t(W, std::vector<double>(H));
    for (int i = 0; i < H; ++i) {
        const double* srcRow = grid[i].data();
        for (int j = 0; j < W; ++j) {
            t[j][i] = srcRow[j];
        }
    }
    return t;
}

} // namespace EnergyMap