#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <d3d11.h>
#include <d2d1.h>
#include <DirectXTex.h>
#include <random>
#include "ppl.h"
#include <algorithm>
#include <WinBase.h>
#include "FlappyData.h"

#define MYRAND (rand())

using namespace std;
using namespace concurrency;
using namespace DirectX;

using Llist = struct {
	int t;
	int f;
};

class SirdsDrawer
{
public:
	// Singleton accessor
	static SirdsDrawer& GetInstance()
	{
		static SirdsDrawer instance;
		return instance;
	}

	// Preserve original signatures so callers in other files don't need changes
	void InitStatics(const ViewingParameters &params, int iWidth_, int iHeight_)
	{
		vd = params.pmm * params.viewDistance / (float)iHeight_;
		os = params.pmm * params.offsetDistance / (float)iHeight_;
		es = params.pmm * params.eyeSeparation / (float)iHeight_;
		width = (float)iWidth_ / 2.f;
		height = (float)iHeight_;
		m_width = iWidth_;
		m_height = iHeight_;
		pmm = params.pmm;
		pixels.resize(m_width * m_height);
		zNear = params.zNear;
		zFar = params.zFar;
	}

	// Same signature as before
	void ZBuffersToDrawer(vector<float>& lzbuf, vector<float>& rzbuf, vector<UINT>& iPixels, bool hidden)
	{
		int widthLocal = m_width;
		int heightLocal = m_height;

		vector<Llist> sameStart;
		sameStart.resize(widthLocal);
		for (int x = 0; x < widthLocal; x++) {
			sameStart[x].t = x;
			sameStart[x].f = x;
		}

		parallel_for(0, heightLocal, [&lzbuf, &rzbuf, &sameStart, this, widthLocal, hidden](int y) {
			vector<Llist> same;
			same = sameStart;
			auto zll = &lzbuf[y * widthLocal];
			auto zlr = &rzbuf[y * widthLocal];
			sirdsnew(zll, zlr, same, hidden);
			SirdsPicAlgo(y, same);
			});
		iPixels = pixels;
	}

private:
	// private ctor to enforce singleton
	SirdsDrawer()
	{
		pixels.clear();
	}

	// preserve copy/move disabled
	SirdsDrawer(const SirdsDrawer&) = delete;
	SirdsDrawer& operator=(const SirdsDrawer&) = delete;

	/* SIRDS algorithm helpers (converted to members) */
	float Lookup(float zp, int x, int& x1) const
	{
		float z = ((-zNear * zFar) / (zFar - zNear))
			/ (zp - 0.5f - (zNear + zFar) / (2.0f * (zFar - zNear)));
		float v;
		if (bReverse)
		{
			v = height * es * (vd - z) / z;
		}
		else
		{
			v = height * es * (z - vd) / z;
		}
		float xvd = ((float)x - width) / height;
		xvd *= z / vd;
		xvd -= es;
		xvd *= vd / z;
		x1 = (int)(xvd*height + width);
		return v;
	}

	void sirdsnew(float* zll, float* zlr, vector<Llist>& same, bool removeHidden)
	{
		int widthLocal = m_width;
		int xInRbuf;
		int xNotUsed;

		for (int left = 0; left < widthLocal; left++) {
			auto s = (int)Lookup(zll[left], left, xInRbuf);
			auto right = left + s;
			if (right > 0 && right < widthLocal) {
				if (xInRbuf > 0 && xInRbuf < widthLocal)
					s -= (int)Lookup(zlr[xInRbuf], xInRbuf, xNotUsed);
				else
					s = 0;
				if (!removeHidden || (s <= 3 && s >= -3)) {
					auto sl = left; 
					auto sr = right;
					for (int st = same[sr].f; st != sl && st != sr; st = same[sr].f) {
						if (st > sl) {
							sr = st;
						}
						else {
							same[sl].t = sr;
							same[sr].f = sl;
							sr = sl;
							sl = st;
						}
					}
					same[sl].t = sr;
					same[sr].f = sl;
				}
			}
		}
	}

	void SirdsPicAlgo(int y, std::vector<Llist>& same)
	{
		UINT32* pv = &pixels[0];
		UINT32* pa = &pv[y * m_width];

		for (UINT x = 0; x < same.size(); x++) {
			if (UINT pixpos = same[x].f;
				pixpos != x) {
				pa[x] = pa[pixpos];
				continue;
			}
			pa[x] = (((rand() & 0xff) > m_Density) ? color1 : color2);
		}
	}

private:
	// member variables converted from previous globals
	float vd = 0;
	float os = 0;
	float es = 0;
	float width = 1400;
	float height = 1000;
	float pmm = 0;
	bool bReverse = false;
	int m_width = 1400;
	int m_height = 1000;
	int m_Density = 127;
	UINT color1 = 0x000000;
	UINT color2 = 0xffffff;
	float zNear;
	float zFar;

	vector<UINT32> pixels;
};

// Backwards-compatible free function wrappers (matching previous declarations)
// These allow existing callers (e.g. Game.cpp) to remain unchanged.

void InitStatics(const ViewingParameters& prams, int iWidth_, int iHeight_)
{
	SirdsDrawer::GetInstance().InitStatics(prams, iWidth_, iHeight_);
}

void ZBuffersToDrawer(vector<float>& lzbuf, vector<float>& rzbuf, vector<UINT>& iPixels, bool hidden)
{
	SirdsDrawer::GetInstance().ZBuffersToDrawer(lzbuf, rzbuf, iPixels, hidden);
}