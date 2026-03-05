#pragma once

#include <vector>
#include <random>

#include "FlappyData.h"

using namespace DirectX;

class IntroScene
{
public:
    IntroScene(GeometricPrimitive* cube,
               GeometricPrimitive* teapot,
               GeometricPrimitive* dodec,
               FlappyData* flappyData,
               float zoffset);

    // Draw/update the intro scene for time 't'.
    // World/View/Projection matrices are supplied by the caller (Game).
    void Draw(float t,
              const XMMATRIX& g_World,
              const XMMATRIX& g_View,
              const XMMATRIX& g_Projection);

    // Reset internal state machine (useful if you want to restart the intro).
    void Reset();

private:
    struct CubeState
    {
        XMFLOAT3 pos;
        XMFLOAT3 vel;
        XMFLOAT3 ang;
        XMFLOAT3 angVel;
        float scale;
    };

    // Parameters (same as original)
    const int kRows = 16;
    const int kCols = 40;
    const float kSpacing = 0.1f;
    const float kCubeBaseScale = 5.0f;
    const float kBreakStart = -1.2f;   // seconds before the wall breaks up
    const float kExplosionSpeedMin = 1.0f;
    const float kExplosionSpeedMax = 2.4f;
    const float kAngularSpeedMax = 6.0f;
    const float kDamping = 0.98f;

    std::vector<CubeState> m_cubes;
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_ur01;
    std::uniform_real_distribution<float> m_urNeg1_1;

    bool m_initialized;
    bool m_explosionInitialized;
    float m_lastT;

    DirectX::GeometricPrimitive* m_cube;
    DirectX::GeometricPrimitive* m_teapot;
    DirectX::GeometricPrimitive* m_dodec;
    FlappyData* m_flappy;
    float m_zoffset;
};