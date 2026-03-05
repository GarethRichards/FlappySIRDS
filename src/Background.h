#pragma once
#include "DirectXTex.h"
#include <string>

namespace SIRDS {

	// Encapsulates configurable background parameters that used to live directly
	// on the Background class.
	class BackgroundConfig {
	public:
		BackgroundConfig() = default;

		int density_ = 64;
		int density2_ = 164;
		int wolframNumber_ = 1236;
		int method_ = 1;
		int pixelSize_ = 2;
		unsigned int color1_ = 0xFF010101;
		unsigned int color2_ = 0xFF00FF00;
		unsigned int color3_ = 0xFF7700FF;
		int hidden_ = 1;
		std::wstring bitmapPath_;
	};

	class Background {
	public:
		Background() :
			// maintain backward-compatible member names as references to the new config
			density_(config_.density_),
			density2_(config_.density2_),
			wolframNumber_(config_.wolframNumber_),
			method_(config_.method_),
			pixelSize_(config_.pixelSize_),
			color1_(config_.color1_),
			color2_(config_.color2_),
			color3_(config_.color3_),
			hidden_(config_.hidden_),
			bitmapPath_(config_.bitmapPath_)
		{
		}

		void SetBackground(DirectX::ScratchImage &&backgroundImage)
		{
			backgroundImage_ = std::move(backgroundImage);
		}

		std::wstring GetBitmapPath() const
		{
			return bitmapPath_;
		}

	public:
		DirectX::ScratchImage backgroundImage_;

		// New dedicated config object
		BackgroundConfig config_;

		// Backwards-compatible aliases (references) so existing code using
		// Background.density_ etc. continues to work.
		int& density_;
		int& density2_;
		int& wolframNumber_;
		int& method_;
		int& pixelSize_;
		unsigned int& color1_;
		unsigned int& color2_;
		unsigned int& color3_;
		int& hidden_;
		std::wstring& bitmapPath_;
	};
}