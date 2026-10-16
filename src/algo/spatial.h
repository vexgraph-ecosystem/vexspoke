#ifndef ALGO_SPATIAL_H
#define ALGO_SPATIAL_H

#include <stdint.h>

// algo/spatial.h — compact spatial indexing helpers.

// Interleave the low 21 bits of each coordinate into a 63-bit Morton code.
uint64_t Spatial_morton3D(uint32_t x, uint32_t y, uint32_t z);

#endif
