// DrawSirdsTo.cpp
// (C) Gareth Richards 2014
// Draws SIRDS to various backgrounds

#include "DrawSirdsTo.h"
#include <algorithm>

using namespace std;
using namespace SIRDS;

DrawSIRDSToBitmap::DrawSIRDSToBitmap() = default;

void DrawSIRDSToBitmap::Init(SIRDS::BackgroundConfig& bg)
{
	m_Density = bg.density_;
	m_Density2 = bg.density2_;
	m_WolframNumber = bg.wolframNumber_;
	m_Method = bg.method_;
	m_PixelSize = bg.pixelSize_;
	color1 = bg.color1_;
	color2 = bg.color2_;
	color3 = bg.color3_;
}

DrawSIRDSToBitmap::~DrawSIRDSToBitmap() = default;

bool DrawSIRDSToBitmap::InParallel()
{
	return m_Method == 1 && m_PixelSize == 1 && m_Density2==0;
}

void DrawSIRDSToBitmap::InitBackground(int width, int height)
{
	(void)width;	// Unused parameter
	(void)height;	// Unused parameter
}

void DrawSIRDSToBitmap::InitPicture(int width, int height, std::function<void(int)> progress)
{
	m_Height = height;
	m_Width = width;
	m_Progress = progress;

	m_picture = make_shared<DirectX::Image>();
	m_picture->height = height;
	m_picture->width = width;
	m_picture->format = DXGI_FORMAT_B8G8R8X8_UNORM;
	m_picture->rowPitch = width*sizeof(UINT);
	m_picture->slicePitch = m_picture->rowPitch * m_picture->height;
	m_picture->pixels = new BYTE[m_picture->slicePitch];
	m_Progress = progress;
}

void DrawSIRDSToBitmap::SirdsPicAlgo1(int y, std::vector<SIRDS::Llist> &same)
{
	auto *pv = reinterpret_cast<UINT32 *>(m_picture->pixels);
	auto*pa = &pv[y * m_Width];
	UINT32 *pam1 = nullptr;
	if (y != 0)
		pam1 = &pv[(y - 1) * m_Width];
	for (UINT x = 0; x < same.size(); x++) {
		if (UINT pixpos = same[x].f;
			pixpos != x){
			pa[x] = pa[pixpos];
			continue;
		}
		if (pam1 != nullptr && !(y % m_PixelSize == 0)) {
			pa[x] = pam1[m_PixelSize*(int)(x / m_PixelSize)];
			continue;
		}
		if (!(x % m_PixelSize == 0)) {
			pa[x] = pa[m_PixelSize*(int)(x / m_PixelSize)];
			continue;
		}
		if (m_Density2 != 0 && y != 0 && 
			(rand() & 0xff) < m_Density2){
				pa[x] = pam1[x];
				continue;
		}
		pa[x] = (((rand() & 0xff) > m_Density) ? color1 : color2);
	}
}

void DrawSIRDSToBitmap::SirdsPicAlgo2(int y, std::vector<SIRDS::Llist> &same)
{
	auto *pv = reinterpret_cast<UINT32 *>(m_picture->pixels);
	auto *pa = &pv[y * m_Width];
	UINT32 *pam1 = nullptr;
	if (y != 0)
		pam1 = &pv[(y - 1) * m_Width];
	for (UINT x = 0; x < same.size(); x++) {
		UINT pixpos = same[x].f;
		if (pixpos != x)
			pa[x] = pa[pixpos];
		else
		{
			if (!(x % m_PixelSize == 0)) {
				pa[x] = pa[m_PixelSize*(int)(x / m_PixelSize)];
				continue;
			}
			if (pam1 != nullptr &&
				!(y % m_PixelSize == 0)) {
				pa[x] = pam1[m_PixelSize*(int)(x / m_PixelSize)];
				continue;
			}
			pa[x] = (((rand() & 0xff) > m_Density) ? color1 : color2);
		}
	}
}

void DrawSIRDSToBitmap::SirdsPicWolfram(int y, std::vector<SIRDS::Llist> &same)
{
	auto *pv = reinterpret_cast<UINT32 *>(m_picture->pixels);
	auto *pa = &pv[y * m_Width];
	UINT32 *pam1 = nullptr;
	if (y != 0)
		pam1 = &pv[(y - 1) * m_Width];
	int x1{ 0 };
	int x2{ 0 };
	int x3{ 0 };
	for (UINT x = 0; x < same.size(); x++) {
		UINT pixpos = same[x].f;
		if (pixpos != x)
			pa[x] = pa[pixpos];
		else
		{
			if (!(x % m_PixelSize == 0)) {
				pa[x] = pa[x - 1];
				continue;
			}
			if (pam1 != nullptr)
			{
				if (!(y % m_PixelSize == 0)) {
					pa[x] = pam1[x];
					continue;
				}
				x1 = pam1[std::max(m_PixelSize*(int)((x - m_PixelSize) / m_PixelSize), 0)] == color1 ? 0 : 1;
				x2 = pam1[x] == color1 ? 0 : 1;
				x3 = pam1[std::min(m_PixelSize*(int)((x + m_PixelSize) / m_PixelSize), m_Width)] == color1 ? 0 : 1;
				auto v = x1 + x2 * 2 + x3 * 4;
				if (auto b = 1 << v;
				(b & m_WolframNumber) == 0)
					pa[x] = color1;
				else
					pa[x] = color2;
				continue;
			}
			pa[x] = (((rand() & 0xff) > m_Density) ? color1 : color2);
		}
	}
}

void DrawSIRDSToBitmap::SirdsPicWolfram3(int y, std::vector<SIRDS::Llist> &same)
{
	UINT32* pv = reinterpret_cast<UINT32*>(m_picture->pixels);
	UINT32* pa = &pv[y * m_Width];
	UINT32* pam1 = nullptr;
	if (y != 0)
		pam1 = &pv[(y - 1) * m_Width];
	int x1(0), x2(0), x3(0);
	for (UINT x = 0; x < same.size(); x++) {
		UINT pixpos = same[x].f;
		if (pixpos != x)
			pa[x] = pa[pixpos];
		else
		{
			if (!(x % m_PixelSize == 0)) {
				pa[x] = pa[x - 1];
				continue;
			}
			if (pam1 != nullptr)
			{
				if (!(y % m_PixelSize == 0)) {
					pa[x] = pam1[x];
					continue;
				}
				auto pm1Col = pam1[std::max(m_PixelSize * (int)((x - m_PixelSize) / m_PixelSize), 0)];
				x1 = pm1Col == color1 ? 0 : pm1Col == color2 ? 1 : 2;
				auto pCol = pam1[x];
				x2 = pCol == color1 ? 0 : pCol == color2 ? 1 : 2;
				auto pp1Col = pam1[std::min(m_PixelSize * (int)((x + m_PixelSize) / m_PixelSize), m_Width)];
				x3 = pp1Col == color1 ? 0 : pCol == color2 ? 1 : 2;

				auto v = x1 + x2 + x3;
				int b = (int)pow(3, v);
				int c = 0;
				for (int i = 0; i < 7; i++)
				{
					int p = (int)pow(3, i);
					int r = int(b / p) % 3;
					int s = int(m_WolframNumber / p) % 3;
					if (r == 1) {
						c = s;
						break;
					}
				}
				if (c == 0)
					pa[x] = color1;
				else if (c == 1)
					pa[x] = color2;
				else
					pa[x] = color3;

				continue;
			}
			auto col = rand() & 0xff;
			pa[x] = ((col < 85) ? color1 : (col < 190) ? color2 : color3);
		}
	}
}

void DrawSIRDSToBitmap::SirdsPicAlgo(int y, std::vector<SIRDS::Llist> &same)
{
	switch (m_Method)
	{
	case 1:
		SirdsPicAlgo1(y, same);
		break;
	case 2:
		SirdsPicAlgo2(y, same);
		break;
	case 3:
		SirdsPicWolfram(y, same);
		break;
	case 4:
		SirdsPicWolfram3(y, same);
		break;
	}
}

std::shared_ptr<DirectX::Image> DrawSIRDSToBitmap::Complete()
{
	return m_picture;
}


DrawSIRDSToColorBitmap::DrawSIRDSToColorBitmap(SIRDS::Background& bg) :
	pv(nullptr),
	m_backgroundImage(bg.backgroundImage_),
	m_scaledBackgroundImage()
{
}

DrawSIRDSToColorBitmap::~DrawSIRDSToColorBitmap()
{
}

bool DrawSIRDSToColorBitmap::InParallel()
{
	return true;
}

void DrawSIRDSToColorBitmap::InitBackground(int width, int height)
{
	m_BackgroundWidth = width;
	m_BackgroundHeight = height;

	m_Width = width;
	m_Height = height;

	DirectX::Resize(m_backgroundImage.GetImages(), 1, m_backgroundImage.GetMetadata(), width, height, 
		DirectX::TEX_FILTER_FORCE_WIC, m_scaledBackgroundImage);
}

void DrawSIRDSToColorBitmap::InitPicture(int width, int height, std::function<void(int)> progress)
{
	m_picture = make_shared<DirectX::Image>();
	m_picture->height = height;
	m_picture->width = width;
	m_picture->format = m_scaledBackgroundImage.GetMetadata().format;
	m_picture->rowPitch = width*sizeof(UINT);
	m_picture->slicePitch = m_picture->rowPitch * m_picture->height;
	m_picture->pixels = new BYTE[m_picture->slicePitch];
	m_Progress = progress;

	pv = m_scaledBackgroundImage.GetPixels();
	if (pv == nullptr)
		throw PictureNotFound("No background defined");
}

void DrawSIRDSToColorBitmap::SirdsPicAlgo(int y, std::vector<SIRDS::Llist> &same)
{
	auto pa0 = reinterpret_cast<int32_t*>(m_picture->pixels);
	auto paback = reinterpret_cast<int32_t*>(pv);
	auto pa = &pa0[y * m_Width];
	auto pBackGround = &paback[(y % m_BackgroundHeight) * m_BackgroundWidth];

	for (int x = 0; x<same.size(); x++) {
		auto pixpos = same[x].f;
		if (pixpos != x)
			pa[x] = pa[pixpos];
		else
			pa[x] = pBackGround[x % m_BackgroundWidth];
	}
}

std::shared_ptr<DirectX::Image> DrawSIRDSToColorBitmap::Complete()
{
	return m_picture;
}
