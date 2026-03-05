#pragma once
#include <vector>
#include <string>
#include <memory>
#include "Background.h"

namespace DirectX {
	struct Image;
}

namespace SIRDS {
	using uschar = unsigned char;

	struct Llist {
		int t;
		int f;
	} ;

	class DrawSirdsInterface 
	{
	public:
		virtual ~DrawSirdsInterface() = default;
		virtual void Init(SIRDS::BackgroundConfig& bg) = 0;
		virtual void InitPicture(int width, int height, std::function<void (int)> progress)=0;
		virtual void InitBackground(int width, int height)=0;
		virtual void SirdsPicAlgo(int y, std::vector<SIRDS::Llist> &same)=0;
		virtual std::shared_ptr<DirectX::Image> Complete() = 0;
		virtual bool InParallel()=0;
		virtual void SetProgress(int progress);
		virtual void sirdsnew(const float *zll, const float *zlr, std::vector<Llist> &same,bool removeHidden);
		std::function<void(int)> m_Progress;
		int m_Width;
		int m_Height;
	};

	class Background;

	class SIRDSDrawer
	{
	public:
		int iWidth_ = 1920;
		int iHeight_ = 1000;
		int rev_ = 1; // crosseyed;
		int iViewingDistance_ = 500;
		int iEyeSeparation_ = 65;
		int iOffset_ = 600;
		int iSlant_;
		float fPMM_ = 5.0f;
		float DPIFactor_ = 1.0f;
		float zShift_ = 0;	
		bool iHidden_ = true;

	protected:
		static SIRDSDrawer *singleSIRDSDrawer;
		Background Backbitmap;

		// Cached parameters (replaces previous globals / namespace params)
		struct CachedParameters {
			float vd = 0.f;       // viewing distance in normalized units
			float os = 0.f;       // offset
			float es = 0.f;       // eye separation
			float centerX = 0.f;  // center X (half width)
			float heightF = 0.f;  // float height
			float pmm = 0.f;      // pixels per mm
			bool bReverse = false;
		} cached_;

		// zNear / zFar are constants used by the depth -> z conversion
		static constexpr float zNear = 1.88976383f;
		static constexpr float zFar  = 4.15748024f;

	public:
		static SIRDSDrawer * GetDrawer() { return singleSIRDSDrawer; }

		SIRDSDrawer()
		{
			singleSIRDSDrawer=this;
			// Some default parameters
		}

		void ZBuffersToDrawer(const std::vector<float> &lzbuf, const std::vector<float> &rzbuf, int width, int height,
			DrawSirdsInterface *pDrawer);
		// Lookup is now an instance method that uses `cached_` populated by InitStatics.
		float Lookup(float value, int x, int& x1);
		void InitStatics();
		bool SafeToSelectObject([[maybe_unused]] int nShapes) const{
			return true;
		}

		Background &GetBackground()
		{
			return Backbitmap;
		}
	};
}