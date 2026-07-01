#ifndef ENERGY_MAP_HPP
#define ENERGY_MAP_HPP

#include "Common.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  EnergyMap  —  Pixel Saliency / Gradient Magnitude Computation
// ─────────────────────────────────────────────────────────────────────────────
//
//  Produces an EnergyGrid in which high values correspond to visually
//  significant regions (edges, textures, faces) and low values correspond to
//  smooth, uniform backgrounds. The DP seam-finder uses this map to route
//  minimum-cost paths through the least-salient pixels.
//
namespace EnergyMap {

    // Computes the full energy map for the given image.
    //
    //  fn == DUAL_GRADIENT  : dual-axis central-difference gradient over RGB.
    //                         Toroidal boundary wrap-around eliminates artificial
    //                         edge artifacts at image borders.
    //  fn == LAPLACIAN      : 5-point discrete Laplacian of the BT.601 luminance
    //                         channel. Stronger response at fine edges.
    //
    //  Throws std::invalid_argument if image is empty.
    EnergyGrid calculateEnergy(const ImageGrid& image,
                               EnergyFunction   fn = EnergyFunction::DUAL_GRADIENT);

    // Transposes a grid so that rows become columns and vice versa.
    // Used to reduce horizontal seam finding to vertical seam finding.
    EnergyGrid transpose(const EnergyGrid& grid);

} // namespace EnergyMap

#endif // ENERGY_MAP_HPP