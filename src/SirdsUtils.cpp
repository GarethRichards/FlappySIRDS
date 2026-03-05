// SirdsUtils.h
// (C) Gareth Richards

#include "pch.h"
#include <vector>
#include <string>
#include <sstream>
#include "SirdsUtils.h"
#include <VertexTypes.h>
#include <CommonStates.h>
#include "SharedResourcePool.h"
#include "Utilities\WaveFrontReader.h"
#include <algorithm>
#include "3DText.h"
#include "ControlMain.h"

using namespace DirectX;
using namespace std;
using namespace Microsoft::WRL;

namespace SIRDS
{

	// Generic split implementation to reduce code duplication
	namespace {
		template <typename StringType, typename StringStreamType, typename CharType>
		std::vector<StringType> split_impl(const StringType &s, CharType delim) {
			StringStreamType ss(s);
			StringType item;
			std::vector<StringType> elems;
			while (std::getline(ss, item, delim)) {
				elems.push_back(item);
			}
			return elems;
		}
	}

	std::vector<std::string> split(const std::string &s, char delim) {
		return split_impl<std::string, std::stringstream, char>(s, delim);
	}

	std::vector<std::wstring> split(const std::wstring &s, wchar_t delim) {
		return split_impl<std::wstring, std::wstringstream, wchar_t>(s, delim);
	}





	namespace MyDX
	{
		void CheckIndexOverflow(size_t value)
		{
			// Use >=, not > comparison, because some D3D level 9_x hardware does not support 0xFFFF index values.
			if (value >= USHRT_MAX)
				throw std::exception("Index value out of range: cannot tesselate primitive so finely");
		}

		// Temporary collection types used when generating the geometry.
		using VertexCollection = std::vector<WaveFrontReader<uint16_t>::Vertex>;
		using IndexCollection = std::vector<uint16_t>;

		// Helper for flipping winding of geometric primitives for LH vs. RH coords
		static void ReverseWinding(IndexCollection& indices, VertexCollection& vertices)
		{
			assert((indices.size() % 3) == 0);
			for (auto it = indices.begin(); it != indices.end(); it += 3)
			{
				std::swap(*it, *(it + 2));
			}

			for (auto it = vertices.begin(); it != vertices.end(); ++it)
			{
				it->textureCoordinate.x = (1.f - it->textureCoordinate.x);
			}
		}


		// Helper for creating a D3D vertex or index buffer.
		template<typename T>
		static void CreateBuffer(_In_ ID3D11Device* device, T const& data, D3D11_BIND_FLAG bindFlags, _Outptr_ ID3D11Buffer** pBuffer)
		{
			assert(pBuffer != 0);

			D3D11_BUFFER_DESC bufferDesc = { 0 };

			bufferDesc.ByteWidth = (UINT)data.size() * sizeof(T::value_type);
			bufferDesc.BindFlags = bindFlags;
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;

			D3D11_SUBRESOURCE_DATA dataDesc = { 0 };

			dataDesc.pSysMem = &data.front();

			ThrowIfFailed(
				device->CreateBuffer(&bufferDesc, &dataDesc, pBuffer)
				);

//			SetDebugObjectName(*pBuffer, "DirectXTK:GeometricPrimitive");
		}


		// Helper for creating a D3D input layout.
		static void CreateInputLayout(_In_ ID3D11Device* device, IEffect* effect, _Outptr_ ID3D11InputLayout** pInputLayout)
		{
			assert(pInputLayout != 0);

			void const* shaderByteCode;
			size_t byteCodeLength;

			effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

			ThrowIfFailed(
				device->CreateInputLayout(VertexPositionNormalTexture::InputElements,
				VertexPositionNormalTexture::InputElementCount,
				shaderByteCode, byteCodeLength,
				pInputLayout)
				);

//			SetDebugObjectName(*pInputLayout, "DirectXTK:GeometricPrimitive");
		}
	}

	using namespace MyDX;





	// Internal ObjPrimative implementation class.
	class ObjPrimative::Impl
	{
	public:
		void Initialize(_In_ ID3D11DeviceContext* deviceContext, VertexCollection& vertices, IndexCollection& indices, bool rhcoords);

		void XM_CALLCONV Draw(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color, _In_opt_ ID3D11ShaderResourceView* texture, bool wireframe, _In_opt_ std::function<void()> setCustomState);

		void Draw(_In_ IEffect* effect, _In_ ID3D11InputLayout* inputLayout, bool alpha, bool wireframe, _In_opt_ std::function<void()> setCustomState);

		void CreateInputLayout(_In_ IEffect* effect, _Outptr_ ID3D11InputLayout** inputLayout);

	private:
		ComPtr<ID3D11Buffer> mVertexBuffer;
		ComPtr<ID3D11Buffer> mIndexBuffer;

		UINT mIndexCount;

		// Only one of these helpers is allocated per D3D device context, even if there are multiple ObjPrimative instances.
		class SharedResources
		{
		public:
			SharedResources(_In_ ID3D11DeviceContext* deviceContext);

			void PrepareForRendering(bool alpha, bool wireframe);

			ComPtr<ID3D11DeviceContext> deviceContext;
			std::unique_ptr<BasicEffect> effect;

			ComPtr<ID3D11InputLayout> inputLayoutTextured;
			ComPtr<ID3D11InputLayout> inputLayoutUntextured;

			std::unique_ptr<CommonStates> stateObjects;
		};


		// Per-device-context data.
		std::shared_ptr<SharedResources> mResources;

		static SharedResourcePool<ID3D11DeviceContext*, SharedResources> sharedResourcesPool;
	};


	// Public entrypoints.
	_Use_decl_annotations_
		void XM_CALLCONV ObjPrimative::Draw(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color, ID3D11ShaderResourceView* texture, bool wireframe, std::function<void()> setCustomState)
	{
		pImpl->Draw(world, view, projection, color, texture, wireframe, setCustomState);
	}


	_Use_decl_annotations_
		void ObjPrimative::Draw(IEffect* effect, ID3D11InputLayout* inputLayout, bool alpha, bool wireframe, std::function<void()> setCustomState)
	{
		pImpl->Draw(effect, inputLayout, alpha, wireframe, setCustomState);
	}


	_Use_decl_annotations_
		void ObjPrimative::CreateInputLayout(IEffect* effect, ID3D11InputLayout** inputLayout)
	{
		pImpl->CreateInputLayout(effect, inputLayout);
	}



	// Global pool of per-device-context ObjPrimative resources.
	SharedResourcePool<ID3D11DeviceContext*, ObjPrimative::Impl::SharedResources> ObjPrimative::Impl::sharedResourcesPool;


	// Per-device-context constructor.
	ObjPrimative::Impl::SharedResources::SharedResources(_In_ ID3D11DeviceContext* deviceContext)
		: deviceContext(deviceContext)
	{
		ComPtr<ID3D11Device> device;
		deviceContext->GetDevice(&device);

		// Create the BasicEffect.
		effect.reset(new BasicEffect(device.Get()));

		effect->EnableDefaultLighting();

		// Create state objects.
		stateObjects.reset(new CommonStates(device.Get()));

		// Create input layouts.
		effect->SetTextureEnabled(true);
		MyDX::CreateInputLayout(device.Get(), effect.get(), &inputLayoutTextured);

		effect->SetTextureEnabled(false);
		MyDX::CreateInputLayout(device.Get(), effect.get(), &inputLayoutUntextured);
	}


	// Sets up D3D device state ready for drawing a primitive.
	void ObjPrimative::Impl::SharedResources::PrepareForRendering(bool alpha, bool wireframe)
	{
		// Set the blend and depth stencil state.
		ID3D11BlendState* blendState;
		ID3D11DepthStencilState* depthStencilState;

		if (alpha)
		{
			// Alpha blended rendering.
			blendState = stateObjects->AlphaBlend();
			depthStencilState = stateObjects->DepthRead();
		}
		else
		{
			// Opaque rendering.
			blendState = stateObjects->Opaque();
			depthStencilState = stateObjects->DepthDefault();
		}

		deviceContext->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);
		deviceContext->OMSetDepthStencilState(depthStencilState, 0);

		// Set the rasterizer state.
		if (wireframe)
			deviceContext->RSSetState(stateObjects->Wireframe());
		else
			deviceContext->RSSetState(stateObjects->CullCounterClockwise());

		ID3D11SamplerState* samplerState = stateObjects->LinearClamp();

		deviceContext->PSSetSamplers(0, 1, &samplerState);
	}


	// Initializes a geometric primitive instance that will draw the specified vertex and index data.
	_Use_decl_annotations_
		void ObjPrimative::Impl::Initialize(ID3D11DeviceContext* deviceContext, VertexCollection& vertices, IndexCollection& indices, bool rhcoords)
	{
		if (vertices.size() >= USHRT_MAX)
			throw std::exception("Too many vertices for 16-bit index buffer");

		if (!rhcoords)
			ReverseWinding(indices, vertices);

		mResources = sharedResourcesPool.DemandCreate(deviceContext);

		ComPtr<ID3D11Device> device;
		deviceContext->GetDevice(&device);

		CreateBuffer(device.Get(), vertices, D3D11_BIND_VERTEX_BUFFER, &mVertexBuffer);
		CreateBuffer(device.Get(), indices, D3D11_BIND_INDEX_BUFFER, &mIndexBuffer);

		mIndexCount = static_cast<UINT>(indices.size());
	}


	// Draws the primitive.
	_Use_decl_annotations_
		void XM_CALLCONV ObjPrimative::Impl::Draw(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMVECTOR color,
		ID3D11ShaderResourceView* texture, bool wireframe, std::function<void()> setCustomState)
	{
		assert(mResources != 0);
		auto effect = mResources->effect.get();
		assert(effect != 0);

		ID3D11InputLayout *inputLayout;
		if (texture)
		{
			effect->SetTextureEnabled(true);
			effect->SetTexture(texture);

			inputLayout = mResources->inputLayoutTextured.Get();
		}
		else
		{
			effect->SetTextureEnabled(false);

			inputLayout = mResources->inputLayoutUntextured.Get();
		}

		float alpha = XMVectorGetW(color);

		// Set effect parameters.
		effect->SetWorld(world);
		effect->SetView(view);
		effect->SetProjection(projection);

		effect->SetDiffuseColor(color);
		effect->SetAlpha(alpha);

		Draw(effect, inputLayout, (alpha < 1.f), wireframe, setCustomState);
	}


	// Draw the primitive using a custom effect.
	_Use_decl_annotations_
		void ObjPrimative::Impl::Draw(IEffect* effect, ID3D11InputLayout* inputLayout, bool alpha, bool wireframe, std::function<void()> setCustomState)
	{
		assert(mResources != 0);
		auto deviceContext = mResources->deviceContext.Get();
		assert(deviceContext != 0);

		// Set state objects.
		mResources->PrepareForRendering(alpha, wireframe);

		// Set input layout.
		assert(inputLayout != 0);
		deviceContext->IASetInputLayout(inputLayout);

		// Activate our shaders, constant buffers, texture, etc.
		assert(effect != 0);
		effect->Apply(deviceContext);

		// Set the vertex and index buffer.
		auto vertexBuffer = mVertexBuffer.Get();
		UINT vertexStride = sizeof(VertexPositionNormalTexture);
		UINT vertexOffset = 0;

		deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexStride, &vertexOffset);

		deviceContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

		// Hook lets the caller replace our shaders or state settings with whatever else they see fit.
		if (setCustomState)
		{
			setCustomState();
		}

		// Draw the primitive.
		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		deviceContext->DrawIndexed(mIndexCount, 0, 0);
	}


	// Create input layout for drawing with a custom effect.
	_Use_decl_annotations_
		void ObjPrimative::Impl::CreateInputLayout(IEffect* effect, ID3D11InputLayout** inputLayout)
	{
		assert(effect != 0);
		assert(inputLayout != 0);

		assert(mResources != 0);
		auto deviceContext = mResources->deviceContext.Get();
		assert(deviceContext != 0);

		ComPtr<ID3D11Device> device;
		deviceContext->GetDevice(&device);

		MyDX::CreateInputLayout(device.Get(), effect, inputLayout);
	}


	//--------------------------------------------------------------------------------------
	// ObjPrimative
	//--------------------------------------------------------------------------------------

	// Constructor.
	ObjPrimative::ObjPrimative()
		: pImpl(new Impl())
	{
	}


	// Destructor.
	ObjPrimative::~ObjPrimative()
	{
	}

	std::unique_ptr<ObjPrimative> ObjPrimative::Load(ID3D11DeviceContext* deviceContext,
		const std::wstring & filename, float &maxPoint, std::function<void(bool, int)> DisplayStatusBar)
	{
		WaveFrontReader<uint16_t> reader;
		ThrowIfFailed(reader.Load(filename.c_str(), true));

		// Create the primitive object.
		std::unique_ptr<ObjPrimative> primitive = std::make_unique<ObjPrimative>();
		for (auto &v : reader.vertices) {
			maxPoint = std::max(maxPoint, v.position.x);
			maxPoint = std::max(maxPoint, v.position.y);
			maxPoint = std::max(maxPoint, v.position.z);
			if (v.normal.x == 0.0 && v.normal.y == 0.0 && v.normal.z == 0.0) {
				v.normal.x = v.position.x;
				v.normal.y = v.position.y;
				v.normal.z = v.position.z;
			}
		}
		// Duplicate indices safely
		std::vector<uint16_t> origIndices = reader.indices;
		for (int i = static_cast<int>(origIndices.size()) - 1; i >= 0; --i) {
			reader.indices.push_back(origIndices[i]);
		}
		primitive->pImpl->Initialize(deviceContext, reader.vertices, reader.indices, true);
		return primitive;
	}
}

using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Concurrency;
using namespace Windows::UI::Core;

namespace SIRDSControl {
	bool TextShape::m_loadingComplete = false;
	shared_ptr<DirectX::GeometricPrimitive> TextShape::m_letterCube;
	unique_ptr<F3DFIGFont> TextShape::myFont;
	std::wstring TextShape::Banner;
	std::mutex mutex;

	void TextShape::Init(ID3D11DeviceContext* deviceContext)
	{
		std::lock_guard<std::mutex> lock(mutex);
		m_letterCube = GeometricPrimitive::CreateCube(deviceContext, 1.0, true);
		if (TextShape::Banner == L"")
		{
			Uri^ uri = ref new Uri("ms-appx:///Assets/Banner.txt");
			auto dispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;
			create_task(StorageFile::GetFileFromApplicationUriAsync(uri)).then([=](task<StorageFile^> t)
			{
				StorageFile^ storageFile = nullptr;
				try {
					storageFile = t.get();
				}
				catch (...)
				{
					return;
				}
				create_task(FileIO::ReadTextAsync(storageFile)).then([=](Platform::String^ fileContent)
				{
					dispatcher->RunAsync(CoreDispatcherPriority::Normal,
						ref new Windows::UI::Core::DispatchedHandler([=]()
					{
						TextShape::Banner = fileContent->Data();
						myFont = std::unique_ptr<F3DFIGFont>(new F3DFIGFont(m_letterCube, TextShape::Banner.c_str()));
						m_loadingComplete = true;
					}));
				});
			});
		}
		else
		{
			myFont = std::unique_ptr<F3DFIGFont>(new F3DFIGFont(m_letterCube, TextShape::Banner.c_str()));
			m_loadingComplete = true;

		}
	}

	void XM_CALLCONV ShapePrimative::Draw(DirectX::FXMMATRIX local, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::FXMVECTOR color,
		ID3D11ShaderResourceView* texture, bool wireframe)
	{
		m_shape->Draw(local, view, projection, color, texture, wireframe);
	}

	void XM_CALLCONV ShapeObj::Draw(DirectX::FXMMATRIX local, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::FXMVECTOR color,
		ID3D11ShaderResourceView* texture, bool wireframe)
	{
		m_obj->Draw(local, view, projection, color, texture, wireframe);
	}

	void XM_CALLCONV TextShape::Draw(DirectX::FXMMATRIX local, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::FXMVECTOR color,
		ID3D11ShaderResourceView* texture, bool wireframe)
	{
		(void) texture;	// Unused parameter
		(void) wireframe;	// Unused parameter
		if (!m_loadingComplete)
			return;
		XMFLOAT4X4 proj, v, w;

		XMStoreFloat4x4(&proj, projection);
		XMStoreFloat4x4(&v, view);
		XMStoreFloat4x4(&w, local);
		myFont->SetProjection(proj);
		myFont->SetView(v);
		myFont->SetWorld(w);
		myFont->SetRows(2);
		float blockSize = w._11;
		myFont->SetBlockSize(blockSize);

		XMFLOAT3 trans;
		trans = XMFLOAT3(0.f, 0.f, 0.f);

		myFont->SetOrientation(m_RoationTransform);
		XMFLOAT4 col;
		XMStoreFloat4(&col, color);
		myFont->SetColor(col);
		myFont->DrawString(trans, m_MeshName);
	}

	void TextShape::SaveInternalState(PersistentState^ state)
	{
		state->SaveInt32(L":Type", 2);
		state->SaveXMFLOAT3(":Scale", m_ScaleTransform);
		state->SaveXMFLOAT3(":Scale2", m_ScaleTransform2);
		state->SaveXMFLOAT3(":Translate", m_TranslateTransform);
		state->SaveXMFLOAT4X4(":Rotate", m_RoationTransform);
		state->SaveString(":Name", ref new Platform::String(m_MeshName.c_str()));
	}

	void TextShape::LoadInternalState(PersistentState^ state)
	{
		m_ScaleTransform = state->LoadXMFLOAT3(":Scale", m_ScaleTransform);
		m_ScaleTransform2 = state->LoadXMFLOAT3(":Scale2", m_ScaleTransform2);
		m_TranslateTransform = state->LoadXMFLOAT3(":Translate", m_TranslateTransform);
		m_RoationTransform = state->LoadXMFLOAT4X4(":Rotate", m_RoationTransform);
		m_MeshName = state->LoadString(":Name", ref new Platform::String(m_MeshName.c_str()))->Data();
	}

	void ShapeObj::SaveInternalState(PersistentState^ state)
	{
		state->SaveInt32(L":Type", 1);
		state->SaveXMFLOAT3(":Scale", m_ScaleTransform);
		state->SaveXMFLOAT3(":Scale2", m_ScaleTransform2);
		state->SaveXMFLOAT3(":Translate", m_TranslateTransform);
		state->SaveXMFLOAT4X4(":Rotate", m_RoationTransform);
		state->SaveString(":Name", ref new Platform::String(m_MeshName.c_str()));
		state->SaveString(":FileName", ref new Platform::String(m_FileName.c_str()));
	}

	void ShapeObj::LoadInternalState(PersistentState^ state)
	{
		m_ScaleTransform = state->LoadXMFLOAT3(":Scale", m_ScaleTransform);
		m_ScaleTransform2 = state->LoadXMFLOAT3(":Scale2", m_ScaleTransform2);
		m_TranslateTransform = state->LoadXMFLOAT3(":Translate", m_TranslateTransform);
		m_RoationTransform = state->LoadXMFLOAT4X4(":Rotate", m_RoationTransform);
		m_MeshName = state->LoadString(":Name", ref new Platform::String(m_MeshName.c_str()))->Data();
		m_FileName = state->LoadString(":FileName", ref new Platform::String(m_FileName.c_str()))->Data();
	}

	void ShapePrimative::SaveInternalState(PersistentState^ state)
	{
		state->SaveInt32(L":Type", 0);
		state->SaveXMFLOAT3(":Scale", m_ScaleTransform);
		state->SaveXMFLOAT3(":Scale2", m_ScaleTransform2);
		state->SaveXMFLOAT3(":Translate", m_TranslateTransform);
		state->SaveXMFLOAT4X4(":Rotate", m_RoationTransform);
		state->SaveString(":Name", ref new Platform::String(m_MeshName.c_str()));
	}

	void ShapePrimative::LoadInternalState(PersistentState^ state)
	{
		m_ScaleTransform = state->LoadXMFLOAT3(":Scale", m_ScaleTransform);
		m_ScaleTransform2 = state->LoadXMFLOAT3(":Scale2", m_ScaleTransform2);
		m_TranslateTransform = state->LoadXMFLOAT3(":Translate", m_TranslateTransform);
		m_RoationTransform = state->LoadXMFLOAT4X4(":Rotate", m_RoationTransform);
		m_MeshName = state->LoadString(":Name", ref new Platform::String(m_MeshName.c_str()))->Data();
	}

	unique_ptr<Shape> ShapeFactory::CreateGeometricShape(_In_ ID3D11DeviceContext* deviceContext, const wstring &name)
	{
		unique_ptr<GeometricPrimitive> shape;
		if (name == L"Cube"){
			shape = GeometricPrimitive::CreateCube(deviceContext, 1.f, true);
		}
		if (name == L"Cone"){
			shape = GeometricPrimitive::CreateCone(deviceContext, 1.f, 1.f, 32, true);
		}
		if (name == L"Cylinder"){
			shape = GeometricPrimitive::CreateCylinder(deviceContext, 1.f, .5f, 32, true);
		}
		if (name == L"Dodecahedron"){
			shape = GeometricPrimitive::CreateDodecahedron(deviceContext, 1.f, true);
		}
		if (name == L"Icosahedron"){
			shape = GeometricPrimitive::CreateIcosahedron(deviceContext, 1.f, true);
		}
		if (name == L"Octahedron"){
			shape = GeometricPrimitive::CreateOctahedron(deviceContext, 1.f, true);
		}
		if (name == L"Teapot"){
			shape = GeometricPrimitive::CreateTeapot(deviceContext, 1.f, 8, true);
		}
		if (name == L"Torus"){
			shape = GeometricPrimitive::CreateTorus(deviceContext, 1.f, .2f, 16, true);
		}
		if (name == L"GeoSphere"){
			shape = GeometricPrimitive::CreateGeoSphere(deviceContext, 1.f, 3, true);
		}
		if (name == L"Sphere"){
			shape = GeometricPrimitive::CreateSphere(deviceContext, 1.f, 3, true);
		}
		if (name == L"Tetrahedron"){
			shape = GeometricPrimitive::CreateTetrahedron(deviceContext, 1.f, true);
		}
		if (shape == nullptr)
			throw L"Failed to find shape";
		return unique_ptr<Shape>(new ShapePrimative(std::move(shape), name));
	}

	unique_ptr<Shape> ShapeFactory::CreateTextShape(_In_ ID3D11DeviceContext* deviceContext, const wstring &Text)
	{
		(void)deviceContext;
		return std::make_unique<TextShape>(Text);
	}

	unique_ptr<Shape> ShapeFactory::LoadObjShape(_In_ ID3D11DeviceContext* deviceContext, const wstring &name, const wstring &fileName, std::function<void(bool, int)> DisplayStatusBar)
	{
		float maxPoint = 0;
		unique_ptr<SIRDS::ObjPrimative> obj = SIRDS::ObjPrimative::Load(deviceContext, fileName, maxPoint, DisplayStatusBar);
		maxPoint = .5f / maxPoint;
		if (DisplayStatusBar != nullptr)
			DisplayStatusBar(false, -1);
		auto aShape = make_unique<ShapeObj>(std::move(obj), name, maxPoint, fileName);
		return move(aShape);
	}

}