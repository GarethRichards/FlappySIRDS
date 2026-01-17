//--------------------------------------------------------------------------------------
// File: SimpleSample.cpp
//
// This is a simple Win32 desktop application showing use of DirectXTK
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <d3d11.h>
#include <d2d1.h>
#include <DirectXTex.h>
#include <directxmath.h>
#include <Audio.h>

#include "CommonStates.h"
#include "DDSTextureLoader.h"
#include "Effects.h"
#include "GeometricPrimitive.h"
#include "Model.h"
#include "PrimitiveBatch.h"
#include "ScreenGrab.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "VertexTypes.h"
#include "ppl.h"

#include "resource.h"
#include <algorithm>
#include <fstream>
#include <vector>
#include <deque>
#include "DebugMe.h"
#include <time.h>
#include "3dText.h"
#include <string>

using namespace DirectX;
using namespace Concurrency;
using namespace std;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE                           g_hInst = nullptr;
HWND                                g_hWnd = nullptr;
D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*                       g_pd3dDevice = nullptr;
ID3D11DeviceContext*                g_pImmediateContext = nullptr;
IDXGISwapChain*                     g_pSwapChain = nullptr;
ID3D11RenderTargetView*             g_pRenderTargetView = nullptr;
ID3D11RenderTargetView*             g_pRenderTargetView2 = nullptr;
ID3D11Texture2D*                    g_pDepthStencil = nullptr;
ID3D11DepthStencilView*             g_pDepthStencilView = nullptr;
ID3D11Texture2D*                    g_pDepthStencil2 = nullptr;
ID3D11DepthStencilView*             g_pDepthStencilView2 = nullptr;
ID3D11ShaderResourceView*           g_SirdsShader = nullptr;

ID3D11ShaderResourceView*           g_pTextureRV1 = nullptr;
ID3D11ShaderResourceView*           g_pTextureRV2 = nullptr;
ID3D11InputLayout*                  g_pBatchInputLayout = nullptr;
ID3D11ShaderResourceView*           g_pZResource = nullptr;
ID3D11ShaderResourceView*           g_pZResource2 = nullptr;
ID3D10EffectShaderResourceVariable* g_pDepthTex = nullptr;

std::unique_ptr<CommonStates>                           g_States;
std::unique_ptr<BasicEffect>                            g_BatchEffect;
std::unique_ptr<EffectFactory>                          g_FXFactory;
std::unique_ptr<GeometricPrimitive>                     g_cylinder;
std::unique_ptr<GeometricPrimitive>                     g_flappy;
std::unique_ptr<GeometricPrimitive>                     g_Rectangle;
std::unique_ptr<GeometricPrimitive>                     g_cube;
std::unique_ptr<GeometricPrimitive>                     g_teapot;
std::unique_ptr<GeometricPrimitive>                     g_dodec;


std::unique_ptr<PrimitiveBatch<VertexPositionColor>>    g_Batch;
std::unique_ptr<SpriteBatch>                            g_Sprites;
std::unique_ptr<SpriteFont>                             g_Font;

std::unique_ptr<F3DFIGFont> myFont;
typedef enum EyeUsed {
	LeftEye,
	RightEye,
	CenterEye
};

typedef enum GameMode {
	Intro,
	Play,
	End
};

GameMode g_Mode;
EyeUsed g_eye;
XMMATRIX                            g_World;
XMMATRIX                            g_Eye;
XMMATRIX                            g_View;
XMMATRIX                            g_Projection;

float g_ViewDistance = 400.f;
float g_OffsetDistance = 200.f;
float g_EyeSeparation = 65.f;
float g_pmm = 0.f;
float g_zNear = 0.f;
float g_zFar = 0.f;
float g_offset = 0.3f;
float flappyy = 0.f;
float flappyv = 0.f;
float flappyx = -.4f;
float m_ls = .025f;
float toffset = 0;
float m_cylinderDiam = .2f;
float m_cylinderHeight = 2.f;
float m_flappyDiam = .1f;
float m_tEnd = 0.f;
float m_animateT = 0.f;
float m_gapHeight = .30f;
int m_score = 0;
int m_HighScore = 0;
int m_iOffset = 0;
float m_gravity = 9.8f;
float m_flapEffect = 3.f;
float m_tLastClick;
float m_lastKeyPress = 0.f;
float lastt = 0;
uint64_t dwTimeStart = 0;
uint64_t dwDeathStart = 0;

std::vector<float> g_leftZBuffer;
std::vector<float> g_rightZBuffer;
std::unique_ptr<AudioEngine> m_audEngine;
std::unique_ptr<SoundEffect> m_GooseSoundEffect;
std::unique_ptr<SoundEffect> m_HeronSoundEffect;
std::unique_ptr<SoundEffect> m_BuzzSoundEffect;
std::deque<float> Columns;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();
bool CheckForCrash(float t);
void DisplayEnd(float t);
void NewGame();
void DoAudio();
float GetElapsedTime();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_SAMPLE1 );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"SampleWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_SAMPLE1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 1920, 1000 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"SampleWindowClass", L"DirectXTK Simple Sample", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                           nullptr );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}

std::ofstream out;
std::ofstream iout;

void Initialize(int w, int h,DXGI_FORMAT format, int samplecount);

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_pRenderTargetView );
    pBackBuffer->Release();

	// Create 2nd render target view
	pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView2);
	pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
	descDepth.Format =  DXGI_FORMAT_R32_TYPELESS;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D( &descDepth, nullptr, &g_pDepthStencil );
    if( FAILED( hr ) )
        return hr;

	hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil2);
	if (FAILED(hr))
		return hr;

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory( &descDSV, sizeof(descDSV) );
    //descDSV.Format = descDepth.Format;
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
        return hr;

	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil2, &descDSV, &g_pDepthStencilView2);
	if (FAILED(hr))
		return hr;

	D3D11_SHADER_RESOURCE_VIEW_DESC srDesc;
	srDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
	srDesc.Texture2D.MostDetailedMip = 0;
	srDesc.Texture2D.MipLevels = 1;

	hr = g_pd3dDevice->CreateShaderResourceView( g_pDepthStencil, &srDesc, &g_pZResource );
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );


	hr = g_pd3dDevice->CreateShaderResourceView(g_pDepthStencil2, &srDesc, &g_pZResource2);
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView2, g_pDepthStencilView2);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

    // Create DirectXTK objects
    g_States.reset( new CommonStates( g_pd3dDevice ) );
    g_Sprites.reset( new SpriteBatch( g_pImmediateContext ) );
    g_FXFactory.reset( new EffectFactory( g_pd3dDevice ) );
    g_Batch.reset( new PrimitiveBatch<VertexPositionColor>( g_pImmediateContext ) );

    g_BatchEffect.reset( new BasicEffect( g_pd3dDevice ) );
    g_BatchEffect->SetVertexColorEnabled(true);

    {
        void const* shaderByteCode;
        size_t byteCodeLength;

        g_BatchEffect->GetVertexShaderBytecode( &shaderByteCode, &byteCodeLength );

        hr = g_pd3dDevice->CreateInputLayout( VertexPositionColor::InputElements,
                                              VertexPositionColor::InputElementCount,
                                              shaderByteCode, byteCodeLength,
                                              &g_pBatchInputLayout );
        if( FAILED( hr ) )
            return hr;
    }
	ID2D1Factory* m_pDirect2dFactory;
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);
	FLOAT dpiX, dpiY;
	m_pDirect2dFactory->GetDesktopDpi( &dpiX, &dpiY );
	g_pmm = dpiX/25.4f;

	g_cylinder = GeometricPrimitive::CreateCylinder(g_pImmediateContext, m_cylinderHeight, m_cylinderDiam, 32U, false);
	g_flappy = GeometricPrimitive::CreateGeoSphere(g_pImmediateContext, m_flappyDiam, 3U, false);
	g_Rectangle = GeometricPrimitive::CreateCube(g_pImmediateContext, 5.f, false);
	g_cube = GeometricPrimitive::CreateCube(g_pImmediateContext, m_ls, false);
	g_teapot = GeometricPrimitive::CreateTeapot(g_pImmediateContext, 1.0f, 8U, false);
	g_dodec = GeometricPrimitive::CreateDodecahedron(g_pImmediateContext, 1.f, false);

    // Initialize the world matrices
    g_World = XMMatrixIdentity();
	NewGame();
	HRSRC hResInfo = FindResource(NULL, MAKEINTRESOURCE(IDR_BANNER_FONT), L"Text");
	HGLOBAL res = LoadResource(NULL, hResInfo);
	const char * fileData = (const char *) LockResource(res);
	myFont = std::unique_ptr<F3DFIGFont>(new F3DFIGFont(g_pImmediateContext, fileData));
	FreeResource(res);
	g_Mode = GameMode::Intro;
	
	AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
	eflags = eflags | AudioEngine_Debug;
#endif
	m_audEngine = unique_ptr<AudioEngine>(new AudioEngine(eflags));
	m_GooseSoundEffect = std::unique_ptr<SoundEffect>(new SoundEffect(m_audEngine.get(), L"Goose_call.wav"));
	m_HeronSoundEffect = std::unique_ptr<SoundEffect>(new SoundEffect(m_audEngine.get(), L"Blue_Heron.wav"));
	m_BuzzSoundEffect = std::unique_ptr<SoundEffect>(new SoundEffect(m_audEngine.get(), L"Buzz.wav"));
	DebugOut() << "Init Device Complete";
	return S_OK;
}

void NewGame()
{
	Columns.clear();
	for (int i = 0; i < 20; i++){
		Columns.push_back((rand() & 0xff) / 330.f - 255.f/660.f);
	}
}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	m_audEngine->Suspend();
	m_GooseSoundEffect = nullptr;
	m_BuzzSoundEffect = nullptr;
	m_HeronSoundEffect = nullptr;
	m_audEngine = nullptr;
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

    if ( g_pBatchInputLayout ) g_pBatchInputLayout->Release();

    if( g_pTextureRV1 ) g_pTextureRV1->Release();
    if( g_pTextureRV2 ) g_pTextureRV2->Release();

    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pDepthStencil ) g_pDepthStencil->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pRenderTargetView2) g_pRenderTargetView2->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}

void OnMouseButtonDown(int xPos,int yPos)
{
	(void)xPos; // Unused parameter
	(void)yPos; // Unused parameter
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;
		case WM_KEYDOWN:
			if (g_Mode != GameMode::Play){
				float tKeyPress = GetElapsedTime();
				if (abs(tKeyPress - m_lastKeyPress) < .4)
					break;
				m_lastKeyPress = tKeyPress;
				m_HeronSoundEffect->Play();
				g_Mode = GameMode::Play;
				dwTimeStart = 0;
				lastt = 0.f;
				flappyy = 0.f;
				flappyv = 0.f;
				toffset = 0;
				m_score = 0;
				m_iOffset = 0;
				NewGame();
			}
			else
			{
				m_GooseSoundEffect->Play();
				flappyv -= m_flapEffect;
			}
			break;
		case WM_LBUTTONDOWN:
			{
			auto xPos = LOWORD(lParam); 
			auto yPos = HIWORD(lParam); 
			if (g_eye==EyeUsed::LeftEye)
				g_eye=EyeUsed::RightEye;
			else
				g_eye=EyeUsed::LeftEye;			
			OnMouseButtonDown(xPos,yPos);
			}
			break;
		case WM_RBUTTONDOWN:
			{
//			auto xPos = LOWORD(lParam); 
//			auto yPos = HIWORD(lParam); 
			g_eye=EyeUsed::CenterEye;
			}
			break;
        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}

void DrawColumn(float x, float y, bool up)
{
	float ys = up ? 1.f : -1.f;
	XMVECTOR vTranslate = XMVectorSet(x, y + ys, 0.6f, 0.f);
	XMMATRIX local = XMMatrixMultiply(g_World, XMMatrixTranslationFromVector(vTranslate));
	g_cylinder->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
}

float zShift=-1.0;
bool increase=false;
void ZBuffersToDrawer(std::vector<float> &lzbuf, std::vector<float> &rzbuf, std::vector<UINT32> &iPixels, bool hidden);
void InitStatics(float iViewingDistance_, int iWidth_, int iHeight_, float iOffset_, float iEyeSeparation_, float fPMM_, float fzNear, float fzFar);
void DisplayIntro(float t);


void DrawScene(float t)
{
	
	XMVECTOR vTranslate = XMVectorSet(flappyx, flappyy, 0.6f, 0.f);
	XMMATRIX local = XMMatrixMultiply(g_World, XMMatrixTranslationFromVector(vTranslate));
	g_flappy->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);

	t *= -1.f;
	int i = 0;
	for (auto c : Columns)
	{
		float x = toffset + 1.f + t + (float)i * .6f;
		DrawColumn(x, c + m_gapHeight/2.f, true);
		DrawColumn(x, c - m_gapHeight/2.f, false);
		i++;
	}

	vTranslate = XMVectorSet(.0, 3.f,  2.5 +.4, 0.f);
	local = XMMatrixMultiply(g_World, XMMatrixTranslationFromVector(vTranslate));
	g_Rectangle->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);

	vTranslate = XMVectorSet(.0, -3.f, 2.5 + .4, 0.f);
	local = XMMatrixMultiply(g_World, XMMatrixTranslationFromVector(vTranslate));
	g_Rectangle->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);

	if (g_Mode == GameMode::Intro)
		DisplayIntro(t);
	if (g_Mode == GameMode::End)
		DisplayEnd(m_animateT);

}

void WobblingText(int rows, float blockSise, float t, float x, float y, float z, const string& text);

void DisplayEnd(float t)
{

	float t2 = sin(8.f*t)*.75f;
	string s = std::to_string(m_score);
	WobblingText(3, .035f, t2, 0, .4f, .5f, s);

	
	m_HighScore = std::max(m_HighScore, m_score);

	string text;
	ULONGLONG ticks = (GetTickCount64() - dwDeathStart);
	int w = ticks / 10000.f;
	if (w % 3 == 1)
	{
		WobblingText(3, .035f, -t2, 0, .1f, .5f, "HIGH");
		WobblingText(3, .035f, -t2, 0, -.2f, .5f, std::to_string(m_HighScore));
	}
	if (w % 3 == 0)
	{
		WobblingText(3, .035f, -t2, 0.1, .0f, .5f, "OUCH!");
	}
	if (w % 3 == 2)
	{
		float x = ticks - w *  10000.f;
		x = 2.f*x/10000.f -1.f;
		XMFLOAT3 translate( x, 0.f, 0.6f );
		XMVECTOR Scaling = XMVectorSet(.5f, .5f, .5f, 5.f);
		XMVECTOR rotateQ = XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(0.f, x * 5, 0.f, 0.f));
		auto local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorSet(0.f, 0.f, 0.f, 0.f), rotateQ,
			XMVectorSet(translate.x, translate.y, translate.z, 0)));
		if (w % 2 == 0)
			g_teapot->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
		else
			g_dodec->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
	}

}

void WobblingText(int rows,float blockSise, float t, float x, float y, float z, const string& text)
{
	XMFLOAT4X4 projectionF;
	XMStoreFloat4x4(&projectionF, g_Projection);
	myFont->SetProjection(projectionF);
	XMFLOAT4X4 viewF;
	XMStoreFloat4x4(&viewF, g_View);
	myFont->SetView(viewF);
	XMFLOAT4X4 worldF;
	XMStoreFloat4x4(&worldF, g_World);
	myFont->SetWorld(worldF);
	myFont->SetRows(rows);
	myFont->SetBlockSize(blockSise);
	

	XMFLOAT3 translate = XMFLOAT3( x-.04f*8.f / 2.f * text.length(), y, z );
	y -= blockSise*4.f;
	auto rotateQ = XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(t, 0.f, 0.f, 0.f));
	auto translate0 = XMVectorSet(0.f, 0.0f, 0.f, 0.f);
	auto RotationOrigin = XMVectorSet(0.f, y, z, 0.f);
	auto Scaling = XMVectorSet(1.f, 1.f, 1.f, 1.f);
	auto rotate = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, RotationOrigin, rotateQ, translate0));
	XMFLOAT4X4 rotateF;
	XMStoreFloat4x4(&rotateF, rotate);
	myFont->SetOrientation(rotateF);
	myFont->DrawString(translate, text);
}

void DisplayIntro(float t)
{
	float t2 = sin(8.f*t)*.6f;

	WobblingText(3, .028f, t2, 0, .3f, .5f, "FLAPPY");
	WobblingText(3, .028f, -t2, 0, .0f, .5f, "DOT");

	XMFLOAT3 translate(.4f, -.25f, 0.5f );
	XMVECTOR Scaling = XMVectorSet(.3f, .3f, .3f, .3f);
	XMVECTOR rotateQ = XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(0.f, t, 0.f, 0.f));
	auto local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorSet(0.f, 0.f, 0.f, 0.f), rotateQ,
		XMVectorSet(translate.x, translate.y, translate.z, 0)));
	g_teapot->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);

	translate = XMFLOAT3(.6, -.3f, 0.5f);
	Scaling = XMVectorSet(.15f, .15f, .15f, .15f);
	rotateQ = XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(0.f, -t*8.f, 0.f, 0.f));
	local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorSet(0.f, 0.f, 0.f, 0.f), rotateQ,
		XMVectorSet(translate.x, translate.y, translate.z, 0)));
	g_dodec->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
}

void RenderToTarget(ID3D11RenderTargetView *RenderTargetView, ID3D11DepthStencilView* DepthStencilView, ID3D11ShaderResourceView* pZResource,float t, bool present)
{
	uint64_t timer1;
	uint64_t timer2;
	uint64_t timer3;
	uint64_t timer4;
	uint64_t timer5;
	timer1 = GetTickCount64();
	g_pImmediateContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;
	float es = g_EyeSeparation * g_pmm / height;
	float vd = g_ViewDistance * g_pmm / height;
	float os = g_OffsetDistance * g_pmm / height;

	// Initialize the view matrix
	float xeye = 1;
	if (g_eye == EyeUsed::LeftEye)
		xeye = -.5f;
	if (g_eye == EyeUsed::RightEye)
		xeye = .5f;
	if (g_eye == EyeUsed::CenterEye)
		xeye = 0;

	g_zNear = vd;
	g_zFar = os + vd;
	// Initialize the view matrix
	XMVECTOR Eye = XMVectorSet(es * xeye, 0.0f, -vd, 0.0f);
	XMVECTOR At = XMVectorSet(es * xeye, 0.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	g_View = XMMatrixLookAtLH(Eye, At, Up);

	g_BatchEffect->SetView(g_View);

	// Initialize the projection matrix
	float angle = atanf(.5f / vd) * 2.f;
	g_Projection = XMMatrixPerspectiveFovLH(angle, (float)width / (float)height, g_zNear, g_zFar);
	//    g_Projection = XMMatrixPerspectiveFovLH( angle, (float)width/(float)height, .01f, 100.f );
	g_BatchEffect->SetProjection(g_Projection);
	//
	// Clear the back buffer
	//
	g_pImmediateContext->ClearRenderTargetView(RenderTargetView, Colors::MidnightBlue);

	//
	// Clear the depth buffer to 1.0 (max depth)
	//
	g_pImmediateContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	
	if (toffset + 1.f + t*-1.0 < -3.f){
		toffset += .6f;
		Columns.pop_front();
		Columns.push_back((rand() & 0xff) / 330.f - 255.f / 660.f);
		m_iOffset++;
	}


	timer2 = GetTickCount64();
	DrawScene(t);
	timer3 = GetTickCount64();
	if (present){
		g_pSwapChain->Present(0, 0);
	}

	g_pImmediateContext->OMSetRenderTargets(1, &RenderTargetView, NULL);

	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	pZResource->GetDesc(&desc);
	ID3D11Resource *pResource;
	pZResource->GetResource(&pResource);
	ScratchImage newImage;
	CaptureTexture(g_pd3dDevice, g_pImmediateContext, pResource, newImage);
	timer3 = GetTickCount64();
	auto Images = newImage.GetImages();
	Image img;
	img.width = Images->width;
	img.height = Images->height;
	img.format = DXGI_FORMAT_D32_FLOAT;
	img.rowPitch = (32 * img.width + 7) / 8;
	img.slicePitch = sizeof(float)*img.width*img.height;
	int bufferSize = img.width*img.height;

	std::vector<float> image(bufferSize, 0.0);
	img.pixels = (uint8_t*)&image[0];
	float *source = (float *)Images->pixels;

	std::vector<float> &zBuffer = ((g_eye == EyeUsed::LeftEye) ? g_leftZBuffer : g_rightZBuffer);
	zBuffer.resize(bufferSize);
	timer4 = GetTickCount64();
	std::copy(source, source + bufferSize, zBuffer.begin());
	timer5 = GetTickCount64();
	//DebugOut() << "Eye," << (float)(timer5 - timer1) / 1000.f
	//	<< "," << (float)(timer2 - timer1) / 1000.f
	//	<< "," << (float)(timer3 - timer2) / 1000.f
	//	<< "," << (float)(timer4 - timer3) / 1000.f
	//	<< "," << (float)(timer5 - timer4) / 1000.f;
/*	if (false)
	{
		float *dest = (float *)img.pixels;
		float Zmin = 100.f;
		float Zmax = 0.f;
		for (int i = 0; i<img.width*img.height; i++){
			if (source[i] != 1)
			{
				Zmax = std::max(zBuffer[i], Zmax);
				Zmin = std::min(zBuffer[i], Zmin);
			}
		}
		//for (int i = 0; i<img.width; i++)
		//{
		//	if (source[i + width*height / 2] != 1.0)
		//	{
		//		out << zShift << "," << g_offset << "," << i << "," << Zmin << "," << Zmax << std::endl;
		//		break;
		//	}
		//}
		g_offset += .1;
		parallel_for(0, bufferSize, [&](int i){
			if (source[i] == 1)
				dest[i] = 0;
			else
			{
				float v = 1 - (zBuffer[i] - Zmin) / (Zmax - Zmin);
				dest[i] = v;
			}
		});
		std::wstring name;
		if (g_eye == EyeUsed::LeftEye){
			name = L"C:\\Work\\TestLeft.jpg";
		}
		else
		{
			name = L"C:\\Work\\TestRight.jpg";
		}

		HRESULT hr = SaveToWICFile(img, WIC_FLAGS_NONE, GetWICCodec(WIC_CODEC_JPEG), name.c_str());
	}*/
}

float GetElapsedTime()
{
	uint64_t dwTimeCur = GetTickCount64();
	if (dwTimeStart == 0)
		dwTimeStart = dwTimeCur;
	return (dwTimeCur - dwTimeStart) / 5000.0f;
}
//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
	DebugOut() << "Starting Render";
	uint64_t timer1;
	uint64_t timer2;
	uint64_t timer3;
	uint64_t timer4;
	uint64_t timer5;
	uint64_t timer6;
	uint64_t timer7;

	timer1 = GetTickCount64();
// Update our time
    static float t = 0.0f;
	t = GetElapsedTime();
	m_animateT = t;
    // Rotate cube around the origin

    g_World = XMMatrixIdentity();

	    // Rotate cube around the origin
    g_Eye = XMMatrixRotationX( t/2 );

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;
	
	if (g_Mode == GameMode::End){
		t = m_tEnd;
	}

	if (g_Mode == GameMode::Play && CheckForCrash(t)){
		g_Mode = GameMode::End;
		m_tEnd = t;
	}
	// Move flappy
	if (lastt != t && g_Mode != GameMode::End)
	{
		float dt = t - lastt;
		flappyv += dt * m_gravity;
		flappyy -= flappyv * dt;
		if (flappyy < -.5f){
			flappyy = -.5f;
			flappyv = 0.f;
		}
	}
	lastt = t;
	g_eye = EyeUsed::LeftEye;
	RenderToTarget(g_pRenderTargetView, g_pDepthStencilView, g_pZResource, t, false);
	timer2 = GetTickCount64();

	g_eye = EyeUsed::RightEye;
	RenderToTarget(g_pRenderTargetView, g_pDepthStencilView, g_pZResource, t, false);
	timer3 = GetTickCount64();

	if (g_rightZBuffer.size()!=0 && g_leftZBuffer.size()!=0)
	{
/*		std::ofstream out2;
		out2.open(L"C:\\Work\\Test2.csv");
		out2 << "vd,es,os,width,height" << std::endl;
		out2 << vd << "," << es << "," << os << "," << width << "," << height << std::endl;
		out2 << "x,left,right" << std::endl;
		for (int i = 0; i<width; i++)
		{			
			out2 << i << "," << g_leftZBuffer[i+width*height/2] << "," << g_rightZBuffer[i+width*height/2] << std::endl;
		}
		*/
		std::vector<UINT32> pixels;
		InitStatics(g_ViewDistance, (int)width, (int)height, g_OffsetDistance, g_EyeSeparation, g_pmm, g_zNear,  g_zFar);
		ZBuffersToDrawer(g_leftZBuffer,g_rightZBuffer,pixels, true);
		timer4 = GetTickCount64();
		ScratchImage sImage;
		Image img;
		img.width=width;
		img.height=height;
		img.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		img.rowPitch = width*sizeof(UINT);
		img.slicePitch = img.rowPitch*img.height;
		img.pixels = (uint8_t *) &pixels[0];
		//      int bufferSize = img.width*img.height;
		//		HRESULT hr = SaveToWICFile( img, WIC_FLAGS_NONE, GetWICCodec(WIC_CODEC_JPEG),L"C:\\Work\\TestHidden.jpg");
		//ZBuffersToDrawer(g_leftZBuffer,g_rightZBuffer,pixels, false);
		//hr = SaveToWICFile( img, WIC_FLAGS_NONE, GetWICCodec(WIC_CODEC_JPEG),L"C:\\Work\\TestNotHidden.jpg");
		sImage.InitializeFromImage(img);
		if (g_SirdsShader != nullptr)
			g_SirdsShader->Release();
		timer5 = GetTickCount64();
		auto hr = DirectX::CreateShaderResourceView(g_pd3dDevice,
			sImage.GetImage(0, 0, 0), 1, sImage.GetMetadata(),
			&g_SirdsShader);
		if (hr != S_OK)
			return;
		timer6 = GetTickCount64();
		g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::Black);
		g_Sprites->Begin(SpriteSortMode_Deferred);
		g_Sprites->Draw(g_SirdsShader, XMFLOAT2(0, 0), nullptr, Colors::White);
		g_Sprites->End();
		timer7 = GetTickCount64();
		g_pSwapChain->Present(0, 0);
		DebugOut() << "Frame," << (float)(timer7 - timer1) / 1000.f
			<< "," << (float)(timer2 - timer1) / 1000.f
			<< "," << (float)(timer3 - timer2) / 1000.f
			<< "," << (float)(timer4 - timer3) / 1000.f
			<< "," << (float)(timer5 - timer4) / 1000.f
			<< "," << (float)(timer6 - timer5) / 1000.f
			<< "," << (float)(timer7 - timer6) / 1000.f;
	}

}

void DoAudio()
{
	
	if (!m_audEngine->Update())
	{
		// No audio device is active
		if (m_audEngine->IsCriticalError())
		{
			
		}
	}
	
}

bool CheckForCrash(float t)
{
	int i = 0;

	if (flappyy + m_flappyDiam / 2.0f > .5f || flappyy - m_flappyDiam / 2.0f < -.5f) // hit roof or floor
	{
		m_score = m_iOffset;
		for (size_t j = 0; j<Columns.size(); j++)
		{
			float x = toffset + 1.f + t*-1.f + (float)j * .6f;
			if (flappyx>x)
				m_score++;
		}
		dwDeathStart = GetTickCount64();
		m_BuzzSoundEffect->Play();
		return true;
	}

	for (auto &c : Columns)
	{
		float x = toffset + 1.f + t*-1.f + (float)i * .6f;
		if ((flappyx - m_flappyDiam/2.f)<(x + m_cylinderDiam / 2) && (flappyx - m_flappyDiam/2.f)>(x - m_cylinderDiam / 2) ||
			(flappyx + m_flappyDiam/2.f)<(x + m_cylinderDiam / 2) && (flappyx + m_flappyDiam/2.f)>(x - m_cylinderDiam / 2)){
			if (!(flappyy + m_flappyDiam / 2.f < c + m_gapHeight / 2.f && flappyy - m_flappyDiam / 2.f > c - m_gapHeight / 2.f)){
				m_score = std::max(i + m_iOffset, 0);
				m_BuzzSoundEffect->Play();
				dwDeathStart = GetTickCount64();
				return true;
			}
		}
		i++;
	}
	return false;
}