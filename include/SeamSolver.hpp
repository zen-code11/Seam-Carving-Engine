#ifndef SEAM_SOLVER_HPP
#define SEAM_SOLVER_HPP

#include "Common.hpp"
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
//  SeamSolver  —  DP Seam Finding, Removal, and Visualization
// ─────────────────────────────────────────────────────────────────────────────
//
//  All three pipeline stages live here:
//    1. findVerticalSeam   — Forward DP pass + backtracking (O(W·H))
//    2. findHorizontalSeam — Transpose trick reduces to vertical case
//    3. removeVerticalSeam — In-place left-shift + pop_back (O(W·H), zero extra heap)
//    4. removeHorizontalSeam — Column-compaction into a new (H-1) grid
//    5. overlayVertical/HorizontalSeam — Marks seam pixels red for visualization
//
namespace SeamSolver {

    // ── Seam Finding ─────────────────────────────────────────────────────────

    // Finds the minimum-energy vertical seam via the DP recurrence:
    //   M(0, j)   = energy(0, j)
    //   M(i, j)   = energy(i, j) + min(M(i-1,j-1), M(i-1,j), M(i-1,j+1))
    //
    // Returns: seam[i] = column index of the seam pixel at row i.
    // Complexity: O(W·H) time, O(W·H) space (cost table).
    std::vector<int> findVerticalSeam(const EnergyGrid& energy);

    // Finds the minimum-energy horizontal seam by transposing the energy grid
    // and delegating to findVerticalSeam.
    //
    // Returns: seam[j] = row index of the seam pixel at column j.
    // Complexity: O(W·H) time.
    std::vector<int> findHorizontalSeam(const EnergyGrid& energy);

    // ── In-Place Seam Removal ────────────────────────────────────────────────

    // Removes one vertical seam from image in-place by left-shifting each row
    // past the seam column and calling pop_back(). Reduces width by exactly 1.
    //
    // Avoids the O(W·H) allocation that a copy-based approach incurs per seam.
    // Complexity: O(W·H) time, O(1) extra space.
    void removeVerticalSeam(ImageGrid& image, const std::vector<int>& seam);

    // Removes one horizontal seam, reducing image height by exactly 1.
    // Because ImageGrid is row-major, horizontal removal requires building a
    // compacted output grid (one allocation of (H-1) × W).
    // Complexity: O(W·H) time, O(W·H) extra space.
    void removeHorizontalSeam(ImageGrid& image, const std::vector<int>& seam);

    // ── Seam Visualization ───────────────────────────────────────────────────

    // Returns a copy of image with the vertical seam highlighted in red (255, 0, 0).
    ImageGrid overlayVerticalSeam(const ImageGrid& image, const std::vector<int>& seam);

    // Returns a copy of image with the horizontal seam highlighted in red (255, 0, 0).
    ImageGrid overlayHorizontalSeam(const ImageGrid& image, const std::vector<int>& seam);

} // namespace SeamSolver

#endif // SEAM_SOLVER_HPP