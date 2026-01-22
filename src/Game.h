#pragma once
#define WINVER 0x0605
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <d2d1.h>
#include <DirectXTex.h>
#include <DirectXMath.h>
#include <Audio.h>
#include <wrl/client.h>

#include "CommonStates.h"
#include "Effects.h"
#include "GeometricPrimitive.h"
#include "PrimitiveBatch.h"
#include "ScreenGrab.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "VertexTypes.h"
#include "ppl.h"
#include "3DText.h"
#include "SimpleMath.h"  // For Matrix types

#include "FlappyData.h"

#include <memory>
#include <vector>
#include <deque>
#include <string>

using Microsoft::WRL::ComPtr;

// Keep short type aliases for existing code compatibility
using EyeUsed = FlappyData::EyeUsed;
using GameMode = FlappyData::GameMode;

const float zoffset = 1.5f;

class Game
{
public:
    Game();
    ~Game();

    // Initialize window and device. Returns S_OK on success.
    HRESULT Initialize(HINSTANCE hInstance, int nCmdShow);

    // Main loop - returns exit code.
    int Run();

    // Shutdown / cleanup
    void Cleanup();

    // Window procedure forwarding
    LRESULT HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    // Initialization helpers
    HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
    HRESULT InitDevice();

    void InitGame();
    void InitAudio();
    void Init3DFont();
    // Render/update
    void Render();
    void RenderToTarget(ID3D11RenderTargetView* RenderTargetView, ID3D11DepthStencilView* DepthStencilView, ID3D11ShaderResourceView* pZResource, float t, bool present);
    float GetElapsedTime();
    void DoAudio();

    // handle window resizes
    void OnResize(UINT width, UINT height);

    // Game logic helpers
    bool CheckForCrash(float t);
    void NewGame();
    void DrawScene(float t);
    void DrawColumn(float x, float y, bool up) const;
    void DisplayIntro(float t);
    void DisplayEnd(float t);
    void WobblingText(int rows, float blockSise, float t, float x, float y, float z, const std::string& text);
    void OnMouseButtonDown(int xPos, int yPos) const;

private:
    // Window / device
    HINSTANCE                           m_hInst = nullptr;
    HWND                                m_hWnd = nullptr;
    D3D_DRIVER_TYPE               m_driverType = D3D_DRIVER_TYPE_NULL;
    D3D_FEATURE_LEVEL             m_featureLevel = D3D_FEATURE_LEVEL_11_0;

    // Replaced raw COM pointers with ComPtr smart wrappers
    ComPtr<ID3D11Device>                       m_pd3dDevice;
    ComPtr<ID3D11DeviceContext>                m_pImmediateContext;
    ComPtr<IDXGISwapChain>                     m_pSwapChain;
    ComPtr<ID3D11RenderTargetView>             m_pRenderTargetView;
    ComPtr<ID3D11RenderTargetView>             m_pRenderTargetView2;
    ComPtr<ID3D11Texture2D>                    m_pDepthStencil;
    ComPtr<ID3D11DepthStencilView>             m_pDepthStencilView;
    ComPtr<ID3D11Texture2D>                    m_pDepthStencil2;
    ComPtr<ID3D11DepthStencilView>             m_pDepthStencilView2;
    ComPtr<ID3D11ShaderResourceView>           m_SirdsShader;

    ComPtr<ID3D11InputLayout>                  m_pBatchInputLayout;
    ComPtr<ID3D11ShaderResourceView>           m_pZResource;
    ComPtr<ID3D11ShaderResourceView>           m_pZResource2;

    std::unique_ptr<DirectX::CommonStates>                           m_States;
    std::unique_ptr<DirectX::BasicEffect>                            m_BatchEffect;
    std::unique_ptr<DirectX::EffectFactory>                          m_FXFactory;
    std::unique_ptr<DirectX::GeometricPrimitive>                     m_cylinder;
    std::unique_ptr<DirectX::GeometricPrimitive>                     m_flappy;
    std::unique_ptr<DirectX::GeometricPrimitive>                     m_Rectangle;
    std::unique_ptr<DirectX::GeometricPrimitive>                     m_cube;
    std::unique_ptr<DirectX::GeometricPrimitive>                     m_teapot;
    std::unique_ptr<DirectX::GeometricPrimitive>                     m_dodec;

    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>    m_Batch;
    std::unique_ptr<DirectX::SpriteBatch>                            m_Sprites;
    std::unique_ptr<F3DFIGFont> myFont;


    DirectX::XMMATRIX                            g_World;
    DirectX::XMMATRIX                            g_Eye;
    DirectX::XMMATRIX                            g_View;
    DirectX::XMMATRIX                            g_Projection;

    // extracted gameplay state moved to its own type and header
    FlappyData flappyData;

    std::vector<float> g_leftZBuffer;
    std::vector<float> g_rightZBuffer;
    std::unique_ptr<DirectX::AudioEngine> m_audEngine;
    std::unique_ptr<DirectX::SoundEffect> m_GooseSoundEffect;
    std::unique_ptr<DirectX::SoundEffect> m_HeronSoundEffect;
    std::unique_ptr<DirectX::SoundEffect> m_BuzzSoundEffect;
    std::deque<float> Columns;

    // Static pointer to current instance for static wndproc
    static Game* s_instance;
};