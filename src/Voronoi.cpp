#include <cstdint>
#include <cmath>
#include <vector>
#include "Voronoi.h"

namespace Voronoi
{

	// --- Voronoi helpers -------------------------------------------------------
	inline uint32_t Hash32(int x, int y, int seed = 0)
	{
		uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u + uint32_t(seed) * 2246822519u;
		h = (h ^ (h >> 13)) * 1274126177u;
		return h ^ (h >> 16);
	}

	// returns pseudo-random [0,1)
	inline float Hash01(int x, int y, int seed = 0)
	{
		return (Hash32(x, y, seed) & 0xFFFFFFu) / float(0x1000000u);
	}


	// Voronoi: nearest site distance and a site id/hash
	void VoronoiNearest(float x, float y, float cellSize, int seed, float& outDist, uint32_t& outSiteHash)
	{
		int gx = int(floorf(x / cellSize));
		int gy = int(floorf(y / cellSize));
		float best = 1e6f;
		uint32_t bestHash = 0;
		for (int oy = -1; oy <= 1; ++oy) {
			for (int ox = -1; ox <= 1; ++ox) {
				int sx = gx + ox;
				int sy = gy + oy;
				// site offset inside cell
				float rx = Hash01(sx, sy, seed + 1) - 0.5f;
				float ry = Hash01(sx, sy, seed + 2) - 0.5f;
				float siteX = (sx + 0.5f + rx) * cellSize;
				float siteY = (sy + 0.5f + ry) * cellSize;
				float dx = siteX - x;
				float dy = siteY - y;
				float d = sqrtf(dx * dx + dy * dy);
				if (d < best) {
					best = d;
					bestHash = Hash32(sx, sy, seed);
				}
			}
		}
		outDist = best;
		outSiteHash = bestHash;
	}

	uint32_t LerpColor(uint32_t c0, uint32_t c1, float t)
	{
		auto lerp8 = [](uint8_t a, uint8_t b, float s)->uint8_t {
			return uint8_t(a + (b - a) * s);
			};
		uint8_t a0 = (c0 >> 24) & 0xFF, r0 = (c0 >> 16) & 0xFF, g0 = (c0 >> 8) & 0xFF, b0 = c0 & 0xFF;
		uint8_t a1 = (c1 >> 24) & 0xFF, r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
		uint32_t a = lerp8(a0, a1, t);
		uint32_t r = lerp8(r0, r1, t);
		uint32_t g = lerp8(g0, g1, t);
		uint32_t b = lerp8(b0, b1, t);
		return (a << 24) | (r << 16) | (g << 8) | b;
	}

	// Map a point to a color from a palette. Use `cellSize` to control Voronoi cell size.
	uint32_t VoronoiColor(float x, float y, float cellSize, int seed, const std::vector<uint32_t>& colors, float blend)
	{
		if (colors.empty()) {
			// default opaque black
			return 0xFF000000u;
		}

		float dist;
		uint32_t siteHash;
		VoronoiNearest(x, y, cellSize, seed, dist, siteHash);

		size_t idx = static_cast<size_t>(siteHash) % colors.size();
		uint32_t c0 = colors[idx];

		if (blend <= 0.0f || colors.size() == 1) {
			return c0;
		}

		// Deterministic pseudo-random t based on the site hash.
		int hx = static_cast<int>(siteHash & 0xFFFFu);
		int hy = static_cast<int>((siteHash >> 16) & 0xFFFFu);
		float t = Hash01(hx, hy, seed + 4);

		// Mix with the next palette entry (wrap).
		size_t idx2 = (idx + 1) % colors.size();
		uint32_t c1 = colors[idx2];

		// blend factor scales how strong the interpolation is
		float finalT = t * blend;
		return LerpColor(c0, c1, finalT);
	}

}
// ---------------------------------------------------------------------------