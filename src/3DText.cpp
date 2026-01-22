
#define NOMINMAX
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include "3dText.h"

using namespace std;
using namespace DirectX;

FigCharacter ReadFontChar(const std::string_view FigData, size_t &current, size_t &next, int charheight);

FIGFont::FIGFont() = default;

void FIGFont::OpenFontFile(const std::string &FigData)
{
	size_t current = 0;
	size_t next = 0;

	next = FigData.find_first_of('\n', current);
	string line = FigData.substr(current, next - current);
	current = next + 1;
	char hardblank;
	std::ignore = sscanf_s(&line[0]+5, "%*c%c %d %*d %d %d %d %d %d",
		&hardblank, 1, &charheight, &maxlen, &smush, &cmtlines,
		&ffright2left, &smush2);
	for (int i = 0; i < cmtlines; i++){
		next = FigData.find_first_of('\n', current);
		current = next + 1;
	}
	long theord;
	for (theord = ' '; theord <= '~'; theord++) {
		FigFont[(int)theord] = ReadFontChar(FigData, current, next, charheight);
	}
	while (next != string::npos)
	{
		next = FigData.find_first_of('\n', current);
		line = FigData.substr(current, next - current);
		current = next + 1;
		if (sscanf_s(line.c_str(), "%li", &theord) == 1) {
			FigFont[(int)theord] = ReadFontChar(FigData, current, next, charheight);
		}
	}
}

FigCharacter FIGFont::GetCharacter(int inChar) const
{	
	auto iter = FigFont.find(inChar);
	return (*iter).second;
}

FigCharacter ReadFontChar(const std::string_view FigData, size_t &current, size_t &next, int charheight)
{
	string line;
	FigCharacter character;
	character.maxWidth = 0;
	for (int row = 0; row<charheight; row++) {
		next = FigData.find_first_of('\n', current);
		line = FigData.substr(current, next - current);
		current = next + 1;
		character.scanline.push_back(line);
		if (line[0] != '\t')
			character.maxWidth = std::max(character.maxWidth, (int)line.length());
	}
	return character;
}

F3DFIGFont::F3DFIGFont(ID3D11DeviceContext* deviceContext, const std::string &FigFontFileName)
{
	font_.OpenFontFile(FigFontFileName);
	cube_ = GeometricPrimitive::CreateCube(deviceContext, 1.f, false);

	rows_ = 2;
}

void F3DFIGFont::DrawString(DirectX::XMFLOAT3 translate,const std::string &Text)
{
	XMFLOAT3 t = translate;
	for (unsigned int i = 0; i < Text.length(); i++)
	{
		int s = DrawCharacter(t, Text[i]);
		t.x = t.x + s * blockSize_;
	}
}

int F3DFIGFont::DrawCharacter(DirectX::XMFLOAT3 translate, int inChar)
{
	auto vTranslateX = XMVectorSet(blockSize_*-1, 0.f, 0.f, 0.f);
	auto vTranslateY = XMVectorSet(0.f, blockSize_*-1, 0.f, 0.f);
	auto vTranslateZ = XMVectorSet(0.f, 0.f, blockSize_*-1, 0.f);
	auto charToWrite = font_.GetCharacter(inChar);
	translate.x += static_cast<float>(charToWrite.maxWidth - 1) * blockSize_;
	auto scale = XMVectorSet(blockSize_, blockSize_, blockSize_, 1.f);
	auto local = XMMatrixMultiply(XMLoadFloat4x4(&world_), XMMatrixScalingFromVector(scale));
	local = XMMatrixMultiply(local, XMMatrixTranslationFromVector(XMVectorSet(translate.x, translate.y, translate.z, 0)));
	for (int k = 0; k < rows_; k++){
		auto localz = local;
		for (auto &row : charToWrite.scanline){
			auto local2 = local;
			for (auto i = static_cast<int>(row.length()) - 1; i >= 0; i--){
				local2 = XMMatrixMultiply(local2, XMMatrixTranslationFromVector(vTranslateX));
				auto local3 = XMMatrixMultiply(local2, XMLoadFloat4x4(&orientation_));
				if (row[i] == '#')
					cube_->Draw(local3, XMLoadFloat4x4(&view_), XMLoadFloat4x4(&projection_), Colors::WhiteSmoke, nullptr);
			}
			local = XMMatrixMultiply(local, XMMatrixTranslationFromVector(vTranslateY));
		}
		local = XMMatrixMultiply(localz, XMMatrixTranslationFromVector(vTranslateZ));
	}
	return charToWrite.maxWidth - 1;
}

void F3DFIGFont::SetView(const DirectX::XMFLOAT4X4 &view)
{
	view_ = view;
}

void F3DFIGFont::SetProjection(const DirectX::XMFLOAT4X4 &projection)
{
	projection_ = projection;
}

void F3DFIGFont::SetWorld(const DirectX::XMFLOAT4X4 &world)
{
	world_ = world;
}


void F3DFIGFont::SetOrientation(const DirectX::XMFLOAT4X4 &orientation)
{
	orientation_ = orientation;
}