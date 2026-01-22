//--------------------------------------------------------------------------------------
// Refactored Game class implementation (was Game.cpp)
//--------------------------------------------------------------------------------------
#include "Game.h"
#include "DDSTextureLoader.h"
#include "Model.h"
#include "ScreenGrab.h"
#include "resource.h"
#include "DebugMe.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <time.h>
#include <array>

using namespace DirectX;
using namespace concurrency;
using namespace std;

// helper to return steady-clock time in milliseconds
static inline uint64_t NowMs()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

void InitStatics(const ViewingParameters &params, int iWidth_, int iHeight_);
void ZBuffersToDrawer(vector<float>& lzbuf, vector<float>& rzbuf, vector<UINT>& iPixels, bool hidden);

Game* Game::s_instance = nullptr;

Game::Game()
{
    s_instance = this;
}

Game::~Game()
{
    s_instance = nullptr;
    Cleanup();
}

HRESULT Game::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    m_hInst = hInstance;
    HRESULT hr = InitWindow(hInstance, nCmdShow);
    if (FAILED(hr)) return hr;

    hr = InitDevice();
    if (FAILED(hr))
    {
        Cleanup();
        return hr;
    }
	InitGame();
    Init3DFont();
	InitAudio();
    return S_OK;
}

int Game::Run()
{
    // Main message loop
    MSG msg = { nullptr };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Render();
        }
    }

    Cleanup();
    return (int)msg.wParam;
}

void Game::Cleanup()
{
    if (m_audEngine) m_audEngine->Suspend();
    m_GooseSoundEffect = nullptr;
    m_BuzzSoundEffect = nullptr;
    m_HeronSoundEffect = nullptr;
    m_audEngine = nullptr;
    if (m_pImmediateContext) m_pImmediateContext->ClearState();

    // Reset COM smart pointers (ComPtr::Reset will release)
    m_pBatchInputLayout.Reset();

    m_pDepthStencilView.Reset();
    m_pDepthStencil.Reset();

    m_pDepthStencilView2.Reset();
    m_pDepthStencil2.Reset();

    m_pRenderTargetView.Reset();
    m_pRenderTargetView2.Reset();
    m_pSwapChain.Reset();
    m_pImmediateContext.Reset();
    m_pd3dDevice.Reset();

    m_States.reset();
    m_Sprites.reset();
    m_FXFactory.reset();
    m_Batch.reset();
    m_BatchEffect.reset();
    m_cylinder.reset();
    m_flappy.reset();
    m_Rectangle.reset();
    m_cube.reset();
    m_teapot.reset();
    m_dodec.reset();
    myFont.reset();
}

HRESULT Game::InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Game::StaticWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_SAMPLE1);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"SampleWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SAMPLE1);
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    RECT rc = { 0, 0, 1920, 1000 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    m_hWnd = CreateWindow(L"SampleWindowClass", L"DirectXTK Simple Sample", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
        nullptr);
    if (!m_hWnd)
        return E_FAIL;

    ShowWindow(m_hWnd, nCmdShow);

    return S_OK;
}

HRESULT Game::InitDevice()
{
    auto hr = S_OK;

    RECT rc;
    GetClientRect(m_hWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    std::array<D3D_DRIVER_TYPE, 3> driverTypes = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    auto numDriverTypes = static_cast<UINT>(driverTypes.size());


    std::array<D3D_FEATURE_LEVEL, 3>  featureLevels = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    auto numFeatureLevels = static_cast<UINT>(featureLevels.size());

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        m_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(nullptr, m_driverType, nullptr, createDeviceFlags, featureLevels.data(), numFeatureLevels,
            D3D11_SDK_VERSION, &sd, m_pSwapChain.GetAddressOf(), m_pd3dDevice.GetAddressOf(), &m_featureLevel, m_pImmediateContext.GetAddressOf());
        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr))
        return hr;

    hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, m_pRenderTargetView.GetAddressOf());
    if (FAILED(hr))
        return hr;
    pBackBuffer->Release();

    // Create 2nd render target view
    pBackBuffer = nullptr;
    hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr))
        return hr;

    hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, m_pRenderTargetView2.GetAddressOf());
    pBackBuffer->Release();
    if (FAILED(hr))
        return hr;

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = m_pd3dDevice->CreateTexture2D(&descDepth, nullptr, m_pDepthStencil.GetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = m_pd3dDevice->CreateTexture2D(&descDepth, nullptr, m_pDepthStencil2.GetAddressOf());
    if (FAILED(hr))
        return hr;

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = DXGI_FORMAT_D32_FLOAT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = m_pd3dDevice->CreateDepthStencilView(m_pDepthStencil.Get(), &descDSV, m_pDepthStencilView.GetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = m_pd3dDevice->CreateDepthStencilView(m_pDepthStencil2.Get(), &descDSV, m_pDepthStencilView2.GetAddressOf());
    if (FAILED(hr))
        return hr;

    D3D11_SHADER_RESOURCE_VIEW_DESC srDesc;
    srDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    srDesc.Texture2D.MostDetailedMip = 0;
    srDesc.Texture2D.MipLevels = 1;

    hr = m_pd3dDevice->CreateShaderResourceView(m_pDepthStencil.Get(), &srDesc, m_pZResource.GetAddressOf());
    if (FAILED(hr))
        return hr;

    // OMSetRenderTargets requires raw pointers - take them temporarily
    {
        ID3D11RenderTargetView* rtv = m_pRenderTargetView.Get();
        m_pImmediateContext->OMSetRenderTargets(1, &rtv, m_pDepthStencilView.Get());
    }

    hr = m_pd3dDevice->CreateShaderResourceView(m_pDepthStencil2.Get(), &srDesc, m_pZResource2.GetAddressOf());
    if (FAILED(hr))
        return hr;

    {
        ID3D11RenderTargetView* rtv2 = m_pRenderTargetView2.Get();
        m_pImmediateContext->OMSetRenderTargets(1, &rtv2, m_pDepthStencilView2.Get());
    }

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_pImmediateContext->RSSetViewports(1, &vp);

    // Create DirectXTK objects
    m_States.reset(new CommonStates(m_pd3dDevice.Get()));
    m_Sprites.reset(new SpriteBatch(m_pImmediateContext.Get()));
    m_FXFactory.reset(new EffectFactory(m_pd3dDevice.Get()));
    m_Batch.reset(new PrimitiveBatch<VertexPositionColor>(m_pImmediateContext.Get()));

    m_BatchEffect.reset(new BasicEffect(m_pd3dDevice.Get()));
    m_BatchEffect->SetVertexColorEnabled(true);

    {
        void const* shaderByteCode;
        size_t byteCodeLength;

        m_BatchEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        hr = m_pd3dDevice->CreateInputLayout(VertexPositionColor::InputElements,
            VertexPositionColor::InputElementCount,
            shaderByteCode, byteCodeLength,
            m_pBatchInputLayout.GetAddressOf());
        if (FAILED(hr))
            return hr;
    }

    ID2D1Factory* m_pDirect2dFactory;
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);

    DebugOut() << "Init Device Complete";
    return S_OK;
}

void Game::Init3DFont()
{
    HRSRC hResInfo = FindResource(nullptr, MAKEINTRESOURCE(IDR_BANNER_FONT), L"Text");
    HGLOBAL res = nullptr;
    const char* fileData = nullptr;
    if (hResInfo)
    {
        res = LoadResource(nullptr, hResInfo);
        if (res)
        {
            fileData = (const char*)LockResource(res);
            if (fileData)
            {
                myFont = std::make_unique<F3DFIGFont>(m_pImmediateContext.Get(), fileData);
            }
            FreeResource(res);
        }
    }
}

void Game::InitGame()
{
    UINT dpi = GetDpiForWindow(m_hWnd); // m_hwnd is your window handle
    auto dpiX = static_cast<float>(dpi);
    flappyData.view.pmm = dpiX / 25.4f;

    m_cylinder = GeometricPrimitive::CreateCylinder(m_pImmediateContext.Get(), flappyData.cylinderHeight, flappyData.cylinderDiam, 32U, false);
    m_flappy = GeometricPrimitive::CreateGeoSphere(m_pImmediateContext.Get(), flappyData.flappyDiam, 3U, false);
    m_Rectangle = GeometricPrimitive::CreateCube(m_pImmediateContext.Get(), 5.f, false);
    m_cube = GeometricPrimitive::CreateCube(m_pImmediateContext.Get(), flappyData.ls, false);
    m_teapot = GeometricPrimitive::CreateTeapot(m_pImmediateContext.Get(), 1.0f, 8U, false);
    m_dodec = GeometricPrimitive::CreateDodecahedron(m_pImmediateContext.Get(), 1.f, false);

    // Initialize the world matrices
    g_World = XMMatrixIdentity();
    NewGame();

    flappyData.mode = GameMode::Intro;
}

void Game::InitAudio()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return;
    m_audEngine = std::make_unique<AudioEngine>();
    m_GooseSoundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"goose_call.wav");
    m_HeronSoundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"blue_heron.wav");
    m_BuzzSoundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"buzz.wav");
}

void Game::NewGame()
{
    Columns.clear();
    for (int i = 0; i < 20; i++) {
        Columns.push_back((rand() & 0xff) / 330.f - 255.f / 660.f);
    }
}

void Game::OnMouseButtonDown(int xPos, int yPos) const
{
    (void)xPos;
    (void)yPos;
}

LRESULT Game::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (s_instance)
    {
        return s_instance->HandleMessage(hWnd, message, wParam, lParam);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT Game::HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;

    switch (message)
    {
    case WM_PAINT:
        std::ignore = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_SIZE:
        // Avoid handling minimize; only resize when device is created.
        if (m_pd3dDevice && m_pSwapChain)
        {
            if (wParam != SIZE_MINIMIZED)
            {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                OnResize(width, height);
            }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_KEYDOWN:
        if (flappyData.mode != GameMode::Play) {
            float tKeyPress = GetElapsedTime();
            if (abs(tKeyPress - flappyData.lastKeyPress) < .4)
                break;
            flappyData.lastKeyPress = tKeyPress;
            m_HeronSoundEffect->Play();
            flappyData.mode = GameMode::Play;
            flappyData.timeStart = 0;
            flappyData.lastT = 0.f;
            flappyData.flappyY = 0.f;
            flappyData.flappyV = 0.f;
            flappyData.toffset = 0;
            flappyData.score = 0;
            flappyData.iOffset = 0;
            NewGame();
        }
        else
        {
            m_GooseSoundEffect->Play();
            flappyData.flappyV -= flappyData.flapEffect;
        }
        break;
    case WM_LBUTTONDOWN:
    {
        auto xPos = LOWORD(lParam);
        auto yPos = HIWORD(lParam);
        if (flappyData.eye == EyeUsed::LeftEye)
            flappyData.eye = EyeUsed::RightEye;
        else
            flappyData.eye = EyeUsed::LeftEye;
        OnMouseButtonDown(xPos, yPos);
    }
    break;
    case WM_RBUTTONDOWN:
    {
        flappyData.eye = EyeUsed::CenterEye;
    }
    break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

void Game::DrawColumn(float x, float y, bool up) const
{
    float ys = up ? 1.f : -1.f;
    XMVECTOR vTranslate = XMVectorSet(x, y + ys, zoffset, 0.f);
    XMMATRIX local = XMMatrixMultiply(g_World, XMMatrixTranslationFromVector(vTranslate));
    m_cylinder->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
}

void Game::DrawScene(float t)
{
    XMVECTOR vTranslate = XMVectorSet(flappyData.flappyX, flappyData.flappyY, zoffset, 0.f);
    XMMATRIX local = XMMatrixMultiply(g_World, XMMatrixTranslationFromVector(vTranslate));
    m_flappy->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);

    t *= -1.f;
    int i = 0;
    for (auto c : Columns)
    {
        float x = flappyData.toffset + 1.f + t + (float)i * .6f;
        DrawColumn(x, c + flappyData.gapHeight / 2.f, true);
        DrawColumn(x, c - flappyData.gapHeight / 2.f, false);
        i++;
    }

    vTranslate = XMVectorSet(.0, 3.3f, 3.5f + .4f, 0.f);
    local = XMMatrixMultiply(g_World, XMMatrixTranslationFromVector(vTranslate));
    m_Rectangle->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);

    vTranslate = XMVectorSet(.0, -3.3f, 3.5f + .4f, 0.f);
    local = XMMatrixMultiply(g_World, XMMatrixTranslationFromVector(vTranslate));
    m_Rectangle->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);

    if (flappyData.mode == GameMode::Intro)
        DisplayIntro(t);
    if (flappyData.mode == GameMode::End)
        DisplayEnd(flappyData.animateT);
}

void Game::WobblingText(int rows, float blockSise, float t, float x, float y, float z, const string& text)
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

    auto translate = XMFLOAT3(x - .04f * 8.f / 2.f * static_cast<float>(text.length()), y, z);
    y -= blockSise * 4.f;
    auto rotateQ = XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(t, 0.f, 0.f, 0.f));
    auto translate0 = XMVectorSet(0.f, 0.0f, 0.f, 0.0f);
    auto RotationOrigin = XMVectorSet(0.f, y, z, 0.f);
    auto Scaling = XMVectorSet(1.f, 1.f, 1.f, 1.f);
    auto rotate = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, RotationOrigin, rotateQ, translate0));
    XMFLOAT4X4 rotateF;
    XMStoreFloat4x4(&rotateF, rotate);
    myFont->SetOrientation(rotateF);
    myFont->DrawString(translate, text);
}

void Game::DisplayEnd(float t)
{
    float t2 = sin(8.f * t)*.75f;
    string s = std::to_string(flappyData.score);
    WobblingText(3, .035f, t2, 0, .4f, zoffset -.1f, s);

    flappyData.highScore = std::max(flappyData.highScore, flappyData.score);

    ULONGLONG ticks = (GetTickCount64() - flappyData.deathStart);
    auto w = ticks / 10000;
    
    if (w % 3 == 1)
    {
        WobblingText(3, .035f, -t2, 0, .1f, zoffset - .1f, "HIGH");
        WobblingText(3, .035f, -t2, 0, -.2f, zoffset - .1f, std::to_string(flappyData.highScore));
    }
    if (w % 3 == 0)
    {
        WobblingText(3, .035f, -t2, 0.1f, .0f, zoffset - .1f, "OUCH!");
    }
    if (w % 2 == 0)
    {
        auto x = static_cast<float>(ticks - w * 10000);
        x = 2.f * x / 10000.f - 1.f;
        XMFLOAT3 translate(x, 0.f, zoffset);
        XMFLOAT3 translate2(x*-1, 0.f, zoffset);
        XMVECTOR rotateQ = XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(0.f, x * 5, 0.f, 0.f));
        if (w % 2 == 0)
        {
            XMVECTOR Scaling = XMVectorSet(.5f, .5f, .5f, .5f);
            auto local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorSet(0.f, 0.f, 0.f, 0.f), rotateQ,
                XMVectorSet(translate.x, translate.y, translate.z, 0)));
            m_teapot->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
            Scaling = XMVectorSet(.3f, .3f, .3f, .3f);
            local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorSet(0.f, 0.f, 0.f, 0.f), rotateQ,
                XMVectorSet(translate2.x, translate2.y, translate2.z, 0)));

            m_dodec->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
        }
        else
        {
            XMVECTOR Scaling = XMVectorSet(.3f, .3f, .3f, .3f);
            auto local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorSet(0.f, 0.f, 0.f, 0.f), rotateQ,
                XMVectorSet(translate.x, translate.y, translate.z, 0)));
            m_dodec->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
        }
    }
}

void Game::DisplayIntro(float t)
{
    float t2 = sin(8.f * t)*.6f;

    WobblingText(3, .028f, t2, 0, .3f, zoffset - 0.1f, "FLAPPY");
    WobblingText(3, .028f, -t2, 0, .0f, zoffset - 0.1f, "DOT");

    XMFLOAT3 translate(-0.9f, -.25f, zoffset - 0.1f);
    XMVECTOR Scaling = XMVectorSet(.3f, .3f, .3f, .3f);
    XMVECTOR rotateQ = XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(0.f, flappyData.animateT, 0.f, 0.f));
    auto local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorSet(0.f, 0.f, 0.f, 0.f), rotateQ,
        XMVectorSet(translate.x, translate.y, translate.z, 0)));
    m_teapot->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);

    translate = XMFLOAT3(.6f, -.3f, zoffset - 0.1f);
    Scaling = XMVectorSet(.15f, .15f, .15f, .15f);
    rotateQ = XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(0.f, -flappyData.animateT * 8.f, 0.f, 0.f));
    local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorSet(0.f, 0.f, 0.f, 0.f), rotateQ,
        XMVectorSet(translate.x, translate.y, translate.z, 0)));
    m_dodec->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
}

uint64_t timer1;
uint64_t timer2;
uint64_t timer3;
uint64_t timer4;
uint64_t timer5;
uint64_t timer6;
uint64_t timer7;

void Game::RenderToTarget(ID3D11RenderTargetView* RenderTargetView, ID3D11DepthStencilView* DepthStencilView, ID3D11ShaderResourceView* pZResource, float t, bool present)
{
    timer1 = NowMs();
    m_pImmediateContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
    RECT rc;
    GetClientRect(m_hWnd, &rc);
    auto width = static_cast<float>(rc.right - rc.left);
    auto height = static_cast<float>(rc.bottom - rc.top);
    float es = flappyData.view.eyeSeparation * flappyData.view.pmm / height;
    float vd = flappyData.view.viewDistance * flappyData.view.pmm / height;
    float os = flappyData.view.offsetDistance * flappyData.view.pmm / height;

    // Initialize the view matrix
    float xeye = 1;
    if (flappyData.eye == EyeUsed::LeftEye)
        xeye = -.5f;
    if (flappyData.eye == EyeUsed::RightEye)
        xeye = .5f;
    if (flappyData.eye == EyeUsed::CenterEye)
        xeye = 0;

    flappyData.view.zNear = vd;
    flappyData.view.zFar = os + vd;
    // Initialize the view matrix
    XMVECTOR Eye = XMVectorSet(es * xeye, 0.0f, -vd, 0.0f);
    XMVECTOR At = XMVectorSet(es * xeye, 0.0f, 0.0f, 0.0f);
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    g_View = XMMatrixLookAtLH(Eye, At, Up);

    m_BatchEffect->SetView(g_View);

    // Initialize the projection matrix
    float angle = atanf(.5f / vd) * 2.f;
    g_Projection = XMMatrixPerspectiveFovLH(angle, width / height, flappyData.view.zNear, flappyData.view.zFar);
    m_BatchEffect->SetProjection(g_Projection);

    m_pImmediateContext->ClearRenderTargetView(RenderTargetView, Colors::MidnightBlue);
    m_pImmediateContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    if (flappyData.toffset + 1.f + t * -1.0 < -3.f) {
        flappyData.toffset += .6f;
        Columns.pop_front();
        Columns.push_back((rand() & 0xff) / 330.f - 255.f / 660.f);
        flappyData.iOffset++;
    }

    timer2 = NowMs();
    DrawScene(t);
    timer3 = NowMs();
    if (present) {
        m_pSwapChain->Present(0, 0);
    }

    m_pImmediateContext->OMSetRenderTargets(1, &RenderTargetView, nullptr);

    D3D11_SHADER_RESOURCE_VIEW_DESC desc;
    pZResource->GetDesc(&desc);
    ID3D11Resource* pResource;
    pZResource->GetResource(&pResource);
    ScratchImage newImage;
    CaptureTexture(m_pd3dDevice.Get(), m_pImmediateContext.Get(), pResource, newImage);
    timer3 = NowMs();
    auto Images = newImage.GetImages();
    auto bufferSize = Images->width * Images->height;
    auto source = (float*)Images->pixels;

    vector<float>& zBuffer = ((flappyData.eye == EyeUsed::LeftEye) ? g_leftZBuffer : g_rightZBuffer);
	if (zBuffer.size() != bufferSize)
        zBuffer.resize(bufferSize);
    timer4 = NowMs();
    std::copy(source, source + bufferSize, zBuffer.begin());
    timer5 = NowMs();
}

float Game::GetElapsedTime()
{
    uint64_t dwTimeCur = GetTickCount64();
    if (flappyData.timeStart == 0)
        flappyData.timeStart = dwTimeCur;
    return static_cast<float>(dwTimeCur - flappyData.timeStart) / 5000.0f;
}

void Game::Render()
{
    DebugOut() << "Starting Render";


    timer1 = NowMs();
    // Update our time
    static float t = 0.0f;
    t = GetElapsedTime();
    flappyData.animateT = t;
    // Rotate cube around the origin

    g_World = XMMatrixIdentity();

        // Rotate cube around the origin
    g_Eye = XMMatrixRotationX(t / 2);

    RECT rc;
    GetClientRect(m_hWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    if (flappyData.mode == GameMode::End) {
        t = flappyData.tEnd;
    }

    if (flappyData.mode == GameMode::Play && CheckForCrash(t)) {
        flappyData.mode = GameMode::End;
        flappyData.tEnd = t;
    }
    // Move flappy
    if (flappyData.lastT != t && flappyData.mode != GameMode::End)
    {
        float dt = t - flappyData.lastT;
        flappyData.flappyV += dt * flappyData.gravity;
        flappyData.flappyY -= flappyData.flappyV * dt;
        if (flappyData.flappyY < -.5f) {
            flappyData.flappyY = -.5f;
            flappyData.flappyV = 0.f;
        }
    }
    flappyData.lastT = t;
    flappyData.eye = EyeUsed::LeftEye;
    RenderToTarget(m_pRenderTargetView.Get(), m_pDepthStencilView.Get(), m_pZResource.Get(), t, false);
    timer2 = NowMs();

    flappyData.eye = EyeUsed::RightEye;
    RenderToTarget(m_pRenderTargetView.Get(), m_pDepthStencilView.Get(), m_pZResource.Get(), t, false);
    timer3 = NowMs();
    if (g_rightZBuffer.empty() || g_leftZBuffer.empty())
		return;
    
    vector<UINT32> pixels;
    InitStatics(flappyData.view, (int)width, (int)height);
    ZBuffersToDrawer(g_leftZBuffer, g_rightZBuffer, pixels, true);
    timer4 = NowMs();
    ScratchImage sImage;
    Image img;
    img.width = width;
    img.height = height;
    img.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    img.rowPitch = width * sizeof(UINT);
    img.slicePitch = img.rowPitch * img.height;
    img.pixels = (uint8_t*)&pixels[0];
    sImage.InitializeFromImage(img);
    if (m_SirdsShader)
        m_SirdsShader.Reset();
    timer5 = NowMs();
    if (auto hr = DirectX::CreateShaderResourceView(m_pd3dDevice.Get(),
        sImage.GetImage(0, 0, 0), 1, sImage.GetMetadata(), m_SirdsShader.GetAddressOf());
        hr != S_OK)
        return;
    timer6 = NowMs();
    m_pImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), Colors::Black);
    m_Sprites->Begin(SpriteSortMode_Deferred);
    m_Sprites->Draw(m_SirdsShader.Get(), XMFLOAT2(0, 0), nullptr, Colors::White);
    m_Sprites->End();
    timer7 = NowMs();
    m_pSwapChain->Present(0, 0);
    DebugOut() << "Frame," << (float)(timer7 - timer1) 
        << ",Start up " << (float)(timer2 - timer1) 
        << ",DrawScene,cap " << (float)(timer3 - timer2) 
        << ",Render " << (float)(timer4 - timer3) 
        << ",copy " << (float)(timer5 - timer4) 
        << ",Finish " << (float)(timer6 - timer5)
        << ",Draw " << (float)(timer7 - timer6)
        << ",FPS " << 1000.f / (float)(timer7 - timer1);
}

void Game::DoAudio()
{
    if (!m_audEngine->Update())
    {
        if (m_audEngine->IsCriticalError())
        {
        }
    }
}

bool Game::CheckForCrash(float t)
{
    int i = 0;

    if (flappyData.flappyY + flappyData.flappyDiam / 2.0f > .8f || flappyData.flappyY - flappyData.flappyDiam / 2.0f < -.8f) // hit roof or floor
    {
        flappyData.score = flappyData.iOffset;
        for (size_t j = 0; j < Columns.size(); j++)
        {
            float x = flappyData.toffset + 1.f + t * -1.f + (float)j * .6f;
            if (flappyData.flappyX > x)
                flappyData.score++;
        }
        flappyData.deathStart = GetTickCount64();
        m_BuzzSoundEffect->Play();
        return true;
    }

    for (const auto& c : Columns)
    {
        if (float x = flappyData.toffset + 1.f + t * -1.f + (float)i * .6f;
            ((flappyData.flappyX - flappyData.flappyDiam / 2.f) < (x + flappyData.cylinderDiam / 2) && (flappyData.flappyX - flappyData.flappyDiam / 2.f) > (x - flappyData.cylinderDiam / 2) ||
            (flappyData.flappyX + flappyData.flappyDiam / 2.f) < (x + flappyData.cylinderDiam / 2) && (flappyData.flappyX + flappyData.flappyDiam / 2.f) > (x - flappyData.cylinderDiam / 2)) &&
            !(flappyData.flappyY + flappyData.flappyDiam / 2.f < c + flappyData.gapHeight / 2.f && flappyData.flappyY - flappyData.flappyDiam / 2.f > c - flappyData.gapHeight / 2.f))
        {
            flappyData.score = std::max(i + flappyData.iOffset, 0);
            m_BuzzSoundEffect->Play();
            flappyData.deathStart = GetTickCount64();
            return true;
        }
        i++;
    }
    return false;
}

void Game::OnResize(UINT width, UINT height)
{
    if (!m_pd3dDevice || !m_pImmediateContext || !m_pSwapChain)
        return;

    // Unbind any render targets
    m_pImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);

    // Release references to views/textures before resizing
    m_pRenderTargetView.Reset();
    m_pRenderTargetView2.Reset();
    m_pDepthStencilView.Reset();
    m_pDepthStencilView2.Reset();
    m_pDepthStencil.Reset();
    m_pDepthStencil2.Reset();
    m_pZResource.Reset();
    m_pZResource2.Reset();

    // Resize swap chain buffers
    HRESULT hr = m_pSwapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    if (FAILED(hr))
    {
        DebugOut() << "ResizeBuffers failed: " << hr;
        return;
    }

    // Recreate render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr) || !pBackBuffer)
    {
        DebugOut() << "GetBuffer failed: " << hr;
        return;
    }

    hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, m_pRenderTargetView.GetAddressOf());
    pBackBuffer->Release();
    if (FAILED(hr))
    {
        DebugOut() << "CreateRenderTargetView failed: " << hr;
        return;
    }

    // Recreate depth stencil textures and views (same desc as in InitDevice)
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;

    hr = m_pd3dDevice->CreateTexture2D(&descDepth, nullptr, m_pDepthStencil.GetAddressOf());
    if (FAILED(hr))
    {
        DebugOut() << "CreateTexture2D depth failed: " << hr;
        return;
    }
    hr = m_pd3dDevice->CreateTexture2D(&descDepth, nullptr, m_pDepthStencil2.GetAddressOf());
    if (FAILED(hr))
    {
        DebugOut() << "CreateTexture2D depth2 failed: " << hr;
        return;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = DXGI_FORMAT_D32_FLOAT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;

    hr = m_pd3dDevice->CreateDepthStencilView(m_pDepthStencil.Get(), &descDSV, m_pDepthStencilView.GetAddressOf());
    if (FAILED(hr))
    {
        DebugOut() << "CreateDepthStencilView failed: " << hr;
        return;
    }
    hr = m_pd3dDevice->CreateDepthStencilView(m_pDepthStencil2.Get(), &descDSV, m_pDepthStencilView2.GetAddressOf());
    if (FAILED(hr))
    {
        DebugOut() << "CreateDepthStencilView2 failed: " << hr;
        return;
    }

    // Recreate SRVs for depth textures (used for depth capture)
    D3D11_SHADER_RESOURCE_VIEW_DESC srDesc;
    srDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srDesc.Texture2D.MostDetailedMip = 0;
    srDesc.Texture2D.MipLevels = 1;

    hr = m_pd3dDevice->CreateShaderResourceView(m_pDepthStencil.Get(), &srDesc, m_pZResource.GetAddressOf());
    if (FAILED(hr))
    {
        DebugOut() << "CreateShaderResourceView depth failed: " << hr;
        return;
    }
    hr = m_pd3dDevice->CreateShaderResourceView(m_pDepthStencil2.Get(), &srDesc, m_pZResource2.GetAddressOf());
    if (FAILED(hr))
    {
        DebugOut() << "CreateShaderResourceView depth2 failed: " << hr;
        return;
    }

    // Bind render target + depth for immediate context (one target at a time is fine)
    {
        ID3D11RenderTargetView* rtv = m_pRenderTargetView.Get();
        m_pImmediateContext->OMSetRenderTargets(1, &rtv, m_pDepthStencilView.Get());
    }
    {
        ID3D11RenderTargetView* rtv2 = m_pRenderTargetView2.Get();
        m_pImmediateContext->OMSetRenderTargets(1, &rtv2, m_pDepthStencilView2.Get());
    }

    // Update viewport
    D3D11_VIEWPORT vp;
    vp.Width = static_cast<FLOAT>(width);
    vp.Height = static_cast<FLOAT>(height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_pImmediateContext->RSSetViewports(1, &vp);
    DebugOut() << "Resized to " << width << "x" << height;
}