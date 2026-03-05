#pragma once
#include <vector>
#include "FlappyData.h"

// Backwards-compatible free-function wrappers implemented in src/Source.cpp
// (Keeps existing call sites unchanged.)
void InitStatics(const ViewingParameters& params, int iWidth_, int iHeight_);
void ZBuffersToDrawer(std::vector<float>& lzbuf, std::vector<float>& rzbuf, std::vector<UINT>& iPixels, bool hidden);