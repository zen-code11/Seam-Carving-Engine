#include "SeamSolver.hpp"
#include "EnergyMap.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace SeamSolver {

// ─────────────────────────────────────────────────────────────────────────────
//  findVerticalSeam
// ─────────────────────────────────────────────────────────────────────────────
//
//  Stage 1 — Forward DP (bottom-up cumulative cost table):
//    M(0, j)   = energy(0, j)
//    M(i, j)   = energy(i, j) + min( M(i-1,j-1), M(i-1,j), M(i-1,j+1) )
//
//  Stage 2 — Anchor:
//    Find j* = argmin{ M(H-1, j) }   (linear scan over the last row)
//
//  Stage 3 — Backtracking:
//    Walk from row H-2 up to row 0; at each step, select whichever of the
//    three parents (left, center, right) holds the smallest accumulated cost.
//
//  BUG FIX (original code): The original backtracking step updated nextCol
//  when selecting the right parent but forgot to update nextCost, making
//  subsequent comparisons incorrect. Both variables are now updated atomically.
//
std::vector<int> findVerticalSeam(const EnergyGrid& energy) {
    const int H = static_cast<int>(energy.size());
    const int W = static_cast<int>(energy[0].size());

    // ── Stage 1: Allocate and fill the DP cost table ─────────────────────────
    CostGrid cost(H, std::vector<double>(W));

    // Base case: top row cost equals its raw energy
    {
        const double* eRow = energy[0].data();
        double*       cRow = cost[0].data();
        for (int j = 0; j < W; ++j) cRow[j] = eRow[j];
    }

    for (int i = 1; i < H; ++i) {
        const double* eRow    = energy[i].data();
        const double* prevRow = cost[i - 1].data();
        double*       cRow    = cost[i].data();

        for (int j = 0; j < W; ++j) {
            // Center parent is always valid
            double best = prevRow[j];
            if (j > 0     && prevRow[j - 1] < best) best = prevRow[j - 1];  // left
            if (j < W - 1 && prevRow[j + 1] < best) best = prevRow[j + 1];  // right
            cRow[j] = eRow[j] + best;
        }
    }

    // ── Stage 2: Locate the minimum-cost anchor in the bottom row ────────────
    const double* lastRow = cost[H - 1].data();
    const int minIdx = static_cast<int>(
        std::min_element(lastRow, lastRow + W) - lastRow
    );

    // ── Stage 3: Backtrack to reconstruct the seam path ──────────────────────
    std::vector<int> seam(H);
    seam[H - 1] = minIdx;

    for (int i = H - 2; i >= 0; --i) {
        const int     currCol = seam[i + 1];
        const double* cRow    = cost[i].data();

        int    bestCol  = currCol;
        double bestCost = cRow[currCol];

        // Left neighbor
        if (currCol > 0 && cRow[currCol - 1] < bestCost) {
            bestCost = cRow[currCol - 1];   // ← update cost AND column together
            bestCol  = currCol - 1;
        }
        // Right neighbor — FIX: original forgot to update bestCost here
        if (currCol < W - 1 && cRow[currCol + 1] < bestCost) {
            bestCost = cRow[currCol + 1];   // ← FIXED (was missing in v1)
            bestCol  = currCol + 1;
        }

        seam[i] = bestCol;
    }

    return seam;
}

// ─────────────────────────────────────────────────────────────────────────────
//  findHorizontalSeam  (Transpose Reduction)
// ─────────────────────────────────────────────────────────────────────────────
//
//  Horizontal seam finding is reduced to the vertical case by transposing
//  the energy grid so that rows and columns exchange roles:
//    transposed[j][i]  =  energy[i][j]
//
//  A vertical seam in the transposed grid gives seam[j] = row index to remove
//  at column j — exactly the semantics needed for removeHorizontalSeam().
//
std::vector<int> findHorizontalSeam(const EnergyGrid& energy) {
    return findVerticalSeam(EnergyMap::transpose(energy));
}

// ─────────────────────────────────────────────────────────────────────────────
//  removeVerticalSeam  (In-Place)
// ─────────────────────────────────────────────────────────────────────────────
//
//  For each row i, shifts all pixels right of seam[i] one position to the
//  left using std::move (O(W) per row) then drops the now-duplicate last
//  element with pop_back().
//
//  KEY OPTIMIZATION vs original:
//    The original version allocated a fresh (H × (W-1)) ImageGrid on every
//    seam iteration — O(N·W·H) total heap allocations for N seams.
//    This in-place approach uses O(1) extra memory regardless of N, which
//    reduces peak RSS and eliminates allocator pressure in the inner loop.
//
void removeVerticalSeam(ImageGrid& image, const std::vector<int>& seam) {
    const int H = static_cast<int>(image.size());
    for (int i = 0; i < H; ++i) {
        auto& row      = image[i];
        const int col  = seam[i];
        // Shift surviving pixels left past the removed column
        std::move(row.begin() + col + 1, row.end(), row.begin() + col);
        row.pop_back();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  removeHorizontalSeam
// ─────────────────────────────────────────────────────────────────────────────
//
//  For each column j, the pixel at row seam[j] is removed and all pixels
//  below it are compacted upward.
//
//  ImageGrid is row-major storage, so a column-wise compaction cannot be done
//  purely in-place without disruptive memory reshuffling. Instead a single
//  output grid of height (H-1) is allocated and filled in one pass.
//  This is still O(W·H) time and O(W·H) space — the same asymptotic cost as
//  the original vertical removal, just unavoidable for row-major → column op.
//
void removeHorizontalSeam(ImageGrid& image, const std::vector<int>& seam) {
    const int H = static_cast<int>(image.size());
    const int W = static_cast<int>(image[0].size());

    ImageGrid result(H - 1, std::vector<Pixel>(W));

    for (int j = 0; j < W; ++j) {
        int destRow = 0;
        for (int i = 0; i < H; ++i) {
            if (i == seam[j]) continue;          // skip the seam pixel
            result[destRow][j] = image[i][j];
            ++destRow;
        }
    }

    image = std::move(result);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Seam Visualization Overlays
// ─────────────────────────────────────────────────────────────────────────────

ImageGrid overlayVerticalSeam(const ImageGrid& image, const std::vector<int>& seam) {
    ImageGrid result = image;   // deep copy
    constexpr Pixel red{255, 0, 0};
    const int H = static_cast<int>(image.size());
    for (int i = 0; i < H; ++i) {
        result[i][seam[i]] = red;
    }
    return result;
}

ImageGrid overlayHorizontalSeam(const ImageGrid& image, const std::vector<int>& seam) {
    ImageGrid result = image;   // deep copy
    constexpr Pixel red{255, 0, 0};
    const int W = static_cast<int>(image[0].size());
    for (int j = 0; j < W; ++j) {
        result[seam[j]][j] = red;
    }
    return result;
}

} // namespace SeamSolver