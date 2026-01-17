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

#define MYRAND (rand())

using namespace std;
using namespace concurrency;
using namespace DirectX;

int density;


typedef struct {
	int t;
	int f;
} Llist;

float vd, os, es, width, height, pmm;
int baseEyeSeparation;
bool bReverse;
int m_width=1400;
int m_height=1000;
int m_PixelSize = 1;
int m_Density2 = 0;
int m_Density = 127;
UINT color1 = 0x000000;
UINT color2 = 0xffffff;
float zNear, zFar;

vector<UINT32> pixels;

void sirdsnew(float *zll, float *zlr, vector<Llist> &same, bool removeHidden);
void SirdsPicAlgo(int y, std::vector<Llist> &same);

void InitStatics(float iViewingDistance_, int iWidth_, int iHeight_, float iOffset_, float iEyeSeparation_, float fPMM_, float fzNear, float fzFar)
{
	vd = fPMM_*iViewingDistance_ / iHeight_ ;
	os = fPMM_*iOffset_ / iHeight_;
	es = fPMM_*iEyeSeparation_ / (iHeight_);
	width = iWidth_ / 2.f;
	height = (float)iHeight_;
	m_width = iWidth_;
	m_height = iHeight_;
	pmm = fPMM_;
	pixels.resize(m_width * m_height);
	zNear = fzNear;
	zFar = fzFar;
}

//parallel_for(0, bufferSize, [&](int i){
//	zBuffer[i] = ((-g_zNear*g_zFar) / (g_zFar - g_zNear))
//		/ (source[i] - 0.5f - (g_zNear + g_zFar) / (2.0f*(g_zFar - g_zNear)));
//});


float Lookup(float zp, int x, int& x1)
{
	float z = ((-zNear*zFar) / (zFar - zNear))
		/ (zp - 0.5f - (zNear + zFar) / (2.0f*(zFar - zNear)));
	float v;
	if (bReverse)
	{
		v = height * es * (vd - z) / z;
	}
	else
	{
		v = height * es * (z - vd) / z;
	}
	float xvd = (float) (x - width) /(float) height;
	xvd *= z / vd;
	xvd -= es;
	xvd *= vd / z;
	x1 = (int)((xvd) * height + width);
	return v;
}


void ZBuffersToDrawer(vector<float> &lzbuf, vector<float> &rzbuf, vector<UINT> &iPixels, bool hidden)
{
	int width = m_width;
	int height = m_height;

	float *zll, *zlr;
	vector<Llist> sameStart;
	sameStart.resize(width);
	for (int x = 0; x<width; x++) {
		sameStart[x].t = x;
		sameStart[x].f = x;
	}
	/*
	for (int y = 0; y < height; y++)
	{
		zll = &lzbuf[y*width];
		zlr = &rzbuf[y*width];
		vector<Llist> same;
		same = sameStart;
		sirdsnew(zll, zlr, same, hidden);
		SirdsPicAlgo(y, same);
	}*/
	parallel_for(0, height, [&](int y){
		vector<Llist> same;
		same = sameStart;
		zll = &lzbuf[y*width];
		zlr = &rzbuf[y*width];
		sirdsnew(zll, zlr, same, hidden);
		SirdsPicAlgo(y, same);
	});
	iPixels = pixels;
}

/* SIRDS algorithm */
void sirdsnew(float *zll, float *zlr, vector<Llist> &same, bool removeHidden)
{
	int s;
	int left, right;
	int st, sl, sr;
	int width = m_width;
	int xInRbuf;
	int xNotUsed;

	for (left = 0; left<width; left++) {
		s = (int)Lookup(zll[left], left, xInRbuf);
		right = left + s;
		if (right > 0 && right < width) {
			if (xInRbuf > 0 && xInRbuf < width)
				s -= (int)Lookup(zlr[xInRbuf], xInRbuf, xNotUsed);
			else
				s = 0;
 			if (!removeHidden || (s <= 3 && s >= -3)) {
				sl = left; sr = right;
				for (st = same[sr].f; st != sl && st != sr; st = same[sr].f) {
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

void SirdsPicAlgo(int y, std::vector<Llist> &same)
{
	UINT32 *pv = &pixels[0];
	UINT32 *pa = &pv[y * m_width];

	for (UINT x = 0; x < same.size(); x++) {
		UINT pixpos = same[x].f;
		if (pixpos != x){
			pa[x] = pa[pixpos];
			continue;
		}
		pa[x] = (((rand() & 0xff) > m_Density) ? color1 : color2);
	}
}