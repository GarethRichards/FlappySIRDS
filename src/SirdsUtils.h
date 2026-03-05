#pragma once
#include <GeometricPrimitive.h>
#include "3DText.h"

namespace SIRDS {
	std::vector<std::string> split(const std::string &s, char delim);
	std::vector<std::wstring> split(const std::wstring &s, wchar_t delim);


	class ObjPrimative {
	public:
		~ObjPrimative();

		static std::unique_ptr<ObjPrimative> Load(ID3D11DeviceContext* deviceContext,
			const std::wstring & data, float & maxPoint, std::function<void(bool, int)> DisplayStatusBar);
		// Draw the primitive.
		void XM_CALLCONV Draw(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection,
			DirectX::FXMVECTOR color = DirectX::Colors::White, _In_opt_ ID3D11ShaderResourceView* texture = nullptr, bool wireframe = false, _In_opt_ std::function<void()> setCustomState = nullptr);

		// Draw the primitive using a custom effect.
		void Draw(_In_ DirectX::IEffect* effect, _In_ ID3D11InputLayout* inputLayout, bool alpha = false, bool wireframe = false, _In_opt_ std::function<void()> setCustomState = nullptr);

		// Create input layout for drawing with a custom effect.
		void CreateInputLayout(_In_ DirectX::IEffect* effect, _Outptr_ ID3D11InputLayout** inputLayout);

	private:
	public:
		ObjPrimative(); // Make constructor public for make_unique
		// Private implementation.
		class Impl;

		std::unique_ptr<Impl> pImpl;

		//prevent copying.
		ObjPrimative(ObjPrimative const&);
		ObjPrimative& operator= (ObjPrimative const&);
	};
}

namespace SIRDSControl {
	class Shape {
	public:
		Shape(const std::wstring &MeshName, float scale = .25f)
			: m_MeshName(MeshName), m_Animation(0)
		{
			DirectX::XMStoreFloat4x4(&m_RoationTransform, DirectX::XMMatrixIdentity());
			m_ScaleTransform = { 1.f, 1.f, 1.f };
			m_ScaleTransform2 = { scale, scale, scale };
			m_TranslateTransform = { 0.0f, 0.0f, 0.75f };
		}
		_Use_decl_annotations_
			virtual void XM_CALLCONV Draw(DirectX::FXMMATRIX local, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::FXMVECTOR color,
			ID3D11ShaderResourceView* texture, bool wireframe) = 0;
		//virtual void SaveInternalState(PersistentState^ state) = 0;
		//virtual void LoadInternalState(PersistentState^ state) = 0;
		virtual int ObjectType() = 0;
		virtual ~Shape() = default;
	protected:
	public:
		DirectX::XMFLOAT4X4  m_RoationTransform;
		DirectX::XMFLOAT3 m_ScaleTransform;
		DirectX::XMFLOAT3 m_ScaleTransform2;
		DirectX::XMFLOAT3 m_TranslateTransform;
		std::wstring m_MeshName;
		int m_Animation;
	};

	class ShapePrimative : public Shape {
	public:

		ShapePrimative(std::unique_ptr<DirectX::GeometricPrimitive> s, const std::wstring &MeshName, float scale = .25f)
			: m_shape(std::move(s)), Shape(MeshName, scale)
		{
		}
	protected:
		std::unique_ptr<DirectX::GeometricPrimitive> m_shape;
	public:
		int ObjectType() { return 1; }
		_Use_decl_annotations_
			virtual void XM_CALLCONV Draw(DirectX::FXMMATRIX local, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::FXMVECTOR color,
			ID3D11ShaderResourceView* texture, bool wireframe);
		//void SaveInternalState(PersistentState^ state);
		//void LoadInternalState(PersistentState^ state);

	};

	class ShapeObj : public Shape {
	public:
		ShapeObj(std::unique_ptr<SIRDS::ObjPrimative> s, const std::wstring &MeshName, float scale, const std::wstring &filename)
			: m_obj(std::move(s)), Shape(MeshName, scale), m_FileName(filename)
		{
		}
		virtual ~ShapeObj() = default;
	protected:
		std::unique_ptr<SIRDS::ObjPrimative> m_obj;
		std::wstring m_FileName;
	public:
		int ObjectType() { return 2; }
		_Use_decl_annotations_
			virtual void XM_CALLCONV Draw(DirectX::FXMMATRIX local, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::FXMVECTOR color,
			ID3D11ShaderResourceView* texture, bool wireframe);
		//void SaveInternalState(PersistentState^ state);
		//void LoadInternalState(PersistentState^ state);
	};

	class TextShape : public Shape {
	public:
		static void Init(ID3D11DeviceContext* deviceContext);
		static std::shared_ptr<DirectX::GeometricPrimitive> m_letterCube;
		static bool m_loadingComplete;
		static std::unique_ptr<F3DFIGFont> myFont;
		static std::wstring Banner;

		TextShape(const std::wstring &MeshName, float scale = .25f)
			: Shape(MeshName, scale)
		{
		}
		int ObjectType() { return 3; }
		virtual void XM_CALLCONV Draw(DirectX::FXMMATRIX local, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::FXMVECTOR color,
			ID3D11ShaderResourceView* texture, bool wireframe);
		//void SaveInternalState(PersistentState^ state);
		//void LoadInternalState(PersistentState^ state);
	};

	class ShapeFactory {
	public:
		static std::unique_ptr<Shape> CreateGeometricShape(_In_ ID3D11DeviceContext* deviceContext, const std::wstring &Name);
		static std::unique_ptr<Shape> CreateTextShape(_In_ ID3D11DeviceContext* deviceContext, const std::wstring &Text);
		static std::unique_ptr<Shape> LoadObjShape(_In_ ID3D11DeviceContext* deviceContext, const std::wstring &name, const std::wstring &fileName, std::function<void(bool, int)> DisplayStatusBar);
	};

}