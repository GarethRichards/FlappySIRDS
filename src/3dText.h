#pragma once

#include <string>
#include <vector>
#include <map>
#include <GeometricPrimitive.h>

#include <DirectXMath.h>

class FigCharacter
{
public:
	std::vector<std::string> scanline;
	int maxWidth;
};

class FIGFont {
public:
	FIGFont();
	void OpenFontFile(const std::string &figFontData);
	FigCharacter GetCharacter(int inChar) const;
	
private:
	int charheight = 0;
	int maxlen = 0;
	int smush = 0;
	int cmtlines = 0;
	int ffright2left = 0;
	int smush2 = 0;
	std::map<int , FigCharacter> FigFont;
};


struct ID3D11DeviceContext;

class F3DFIGFont
{
public:
	F3DFIGFont(ID3D11DeviceContext* deviceContext, const std::string &figFontData);

	void SetView(const DirectX::XMFLOAT4X4 &view);
	void SetRows(int rows) { rows_ = rows; }
	void SetProjection(const DirectX::XMFLOAT4X4 &projection);
	void SetWorld(const DirectX::XMFLOAT4X4 &world);
	void SetOrientation(const DirectX::XMFLOAT4X4 &orientation);
	void DrawString(const DirectX::XMFLOAT3 translate, const std::string &Text);
	int DrawCharacter(const DirectX::XMFLOAT3 translate, int inChar);
	void SetBlockSize(float size) { blockSize_ = size; }
private:
	DirectX::XMFLOAT4X4 projection_;
	DirectX::XMFLOAT4X4 view_;
	DirectX::XMFLOAT4X4 world_;
	DirectX::XMFLOAT4X4 orientation_;
	FIGFont font_;
	float blockSize_ = 0;
	int rows_ = 0;
	std::unique_ptr<DirectX::GeometricPrimitive> cube_;
};