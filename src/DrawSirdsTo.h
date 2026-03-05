#pragma once

#include "Background.h"
#include <vector>
#include "SirdsUtils.h"
#include "DrawSirds.h"

namespace SIRDS {
	struct PictureNotFound : public std::exception {
		using std::exception::exception;
	};

	class DrawSIRDSToBitmap : public SIRDS::DrawSirdsInterface
	{
		std::shared_ptr<DirectX::Image> m_picture;
		int m_Density;
		int m_Density2;
		int m_WolframNumber;
		int m_Method;
		int m_PixelSize;
		UINT32 color1;
		UINT32 color2;
		UINT32 color3;
	public:

		DrawSIRDSToBitmap();
		~DrawSIRDSToBitmap() override;

		void Init(SIRDS::BackgroundConfig& bg);
		bool InParallel();
		void InitBackground(int width, int height);
		void InitPicture(int width, int height, std::function<void(int)> progress);
		void SirdsPicAlgo1(int y, std::vector<SIRDS::Llist> &same);
		void SirdsPicAlgo2(int y, std::vector<SIRDS::Llist> &same);
		void SirdsPicWolfram(int y, std::vector<SIRDS::Llist> &same);
		void SirdsPicWolfram3(int y, std::vector<SIRDS::Llist> &same);
		void SirdsPicAlgo(int y, std::vector<SIRDS::Llist> &same);
		std::shared_ptr<DirectX::Image> Complete();
	};


	class DrawSIRDSToColorBitmap : public SIRDS::DrawSirdsInterface
	{
		DirectX::ScratchImage & m_backgroundImage;
		DirectX::ScratchImage m_scaledBackgroundImage;
		std::shared_ptr<DirectX::Image> m_picture;
		UINT m_BackgroundWidth;
		UINT m_BackgroundHeight;
		BYTE *pv;
	public:
		DrawSIRDSToColorBitmap(SIRDS::Background& bg);
		virtual ~DrawSIRDSToColorBitmap();
		bool InParallel();
		void InitBackground(int width, int height);
		void InitPicture(int width, int height, std::function<void(int)> progress);
		void SirdsPicAlgo(int y, std::vector<SIRDS::Llist> &same);
		std::shared_ptr<DirectX::Image> Complete();
	};

}