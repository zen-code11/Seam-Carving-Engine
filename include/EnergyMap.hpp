#ifndef ENERGY_MAP_HPP
#define ENERGY_MAP_HPP

#include "Common.hpp"

namespace EnergyMap {
    // Computes the dual-gradient energy map for the entire native image grid
    EnergyGrid calculateEnergy(const ImageGrid& image);
}

#endif // ENERGY_MAP_HPP