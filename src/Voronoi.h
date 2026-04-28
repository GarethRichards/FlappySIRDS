#pragma once

#include <cstdint>
#include <vector>

namespace Voronoi
{
	uint32_t LerpColor(uint32_t c0, uint32_t c1, float t);
	void VoronoiNearest(float x, float y, float cellSize, int seed, float& outDist, uint32_t& outSiteHash);

	// Return a color for point (x,y) using the provided palette.
	// - `cellSize` controls Voronoi cell size (same as VoronoiNearest).
	// - `seed` is used for deterministic placement/randomization.
	// - `colors` is the palette (32-bit ARGB values).
	// - `blend` in [0,1] controls how much to mix with a neighbor palette color (0 = no blend).
	uint32_t VoronoiColor(float x, float y, float cellSize, int seed, const std::vector<uint32_t>& colors, float blend = 0.0f);
}