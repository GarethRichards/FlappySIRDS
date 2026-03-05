#include <DirectXMath.h>
#include <DirectXColors.h>
#include <random>
#include "DrawSirds.h"
#include <algorithm>
#include "DrawSirdsTo.h"
#include <fstream>
#include "ppl.h"

using namespace std;
using namespace concurrency;
using namespace DirectX;
using namespace Concurrency;

namespace SIRDS
{
	void DrawSirdsInterface::SetProgress(int progress)
	{
		if (m_Progress != nullptr)
			m_Progress(progress);
	}

	SIRDSDrawer* SIRDSDrawer::singleSIRDSDrawer;

	void SIRDSDrawer::InitStatics()
	{
		// Populate the cached parameters used by Lookup and other code.
		cached_.vd = DPIFactor_ * fPMM_ * iViewingDistance_ / static_cast<float>(iHeight_);
		cached_.os = fPMM_ * iOffset_ / static_cast<float>(iHeight_);
		cached_.es = fPMM_ * iEyeSeparation_ / static_cast<float>(iHeight_);
		cached_.centerX = iWidth_ / 2.f;
		cached_.heightF = static_cast<float>(iHeight_);
		cached_.pmm = fPMM_;
		cached_.bReverse = (rev_ == -1);
	}

	float SIRDSDrawer::Lookup(float zp, int x, int& x1)
	{
		// Use the instance cached parameters.
		// Caller must ensure InitStatics() was called if members have changed.
		// Defensive fallback if vd is zero to avoid division by zero:
		const float vd = cached_.vd != 0.f ? cached_.vd : (DPIFactor_ * fPMM_ * iViewingDistance_ / static_cast<float>(iHeight_));
		const float es = cached_.es;
		const float height = cached_.heightF != 0.f ? cached_.heightF : static_cast<float>(iHeight_);
		const float centerX = cached_.centerX != 0.f ? cached_.centerX : (iWidth_ / 2.f);
		const bool bReverse = cached_.bReverse;

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
		if (x > 0) {
			float xvd = (static_cast<float>(x) - centerX) / height;
			xvd *= (z / vd);
			xvd -= es;
			xvd *= (vd / z);
			x1 = static_cast<int>(xvd * height + centerX);
		}
		return v;
	}

	void SIRDSDrawer::ZBuffersToDrawer(const vector<float>& lzbuf, const vector<float>& rzbuf, int iWidth, int iHeight,
		DrawSirdsInterface* pDrawer)
	{
		// Update the integer members to match the provided dimensions before init.
		iWidth_ = iWidth;
		iHeight_ = iHeight;

		InitStatics();
		auto& leftBuf = lzbuf;
		auto& rightBuf = rzbuf;

		vector<Llist> sameStart(iWidth);
		for (int x = 0; x < iWidth; x++) {
			sameStart[x].t = x;
			sameStart[x].f = x;
		}
		if (pDrawer->InParallel())
		{
			parallel_for(0, iHeight, [&](int y) {
				vector<Llist> same;
				same = sameStart;
				auto zll = &leftBuf[y * iWidth];
				auto zlr = &rightBuf[y * iWidth];
				pDrawer->sirdsnew(zll, zlr, same, iHidden_);
				pDrawer->SirdsPicAlgo(y, same);
			});
		}
		else
		{
			for (int y = 0; y < iHeight; y++) {
				auto zll = &lzbuf[y * iWidth];
				auto zlr = &rzbuf[y * iWidth];
				vector<Llist> same;
				same = sameStart;
				pDrawer->sirdsnew(zll, zlr, same, iHidden_);
				pDrawer->SirdsPicAlgo(y, same);
			}
		}

		pDrawer->Complete();
		pDrawer->SetProgress(iHeight * 3);
	}

	/* SIRDS algorithm */
	void DrawSirdsInterface::sirdsnew(const float* zll, const float* zlr, vector<Llist>& same, bool removeHidden)
	{
		int xInRbuf  = 0;
		int xNotUsed = 0;

		// Get drawer instance once to call the instance Lookup method.
		SIRDSDrawer* drawer = SIRDSDrawer::GetDrawer();

		for (int left = 0; left < m_Width; left++) {
			int s = 0;
			if (drawer) s = static_cast<int>(drawer->Lookup(zll[left], left, xInRbuf));
			else s = 0;

			int right = left + s;
			if (right > 0 && right < m_Width) {
				if (xInRbuf > 0 && xInRbuf < m_Width) {
					if (drawer)
						s -= static_cast<int>(drawer->Lookup(zlr[xInRbuf], -1, xNotUsed));
					else
						s = 0;
				}
				else
					s = 0;

				if (!removeHidden || (s <= 3 && s >= -3)) {
					auto sl = left;
					auto sr = right;
					for (auto st = same[sr].f; st != sl && st != sr; st = same[sr].f) {
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
}