#pragma once

#include <vector>
#include <random>
#include <DirectXMath.h>

#include "FlappyData.h"

using namespace DirectX;

class SpiralIntro
{
public:
    SpiralIntro(GeometricPrimitive* cube,
                GeometricPrimitive* teapot,
                GeometricPrimitive* dodec,
                FlappyData* flappyData,
                float zoffset);

    // Draw/update the spiral intro for time 't'.
    // Matrices are supplied by the caller.
    void Draw(float t,
              const XMMATRIX& g_World,
              const XMMATRIX& g_View,
              const XMMATRIX& g_Projection);

    void Reset();

private:
    struct SpiralCube
    {
        float angle;
        float radius;
        float y;
        float z;
        float angVel;
        float radialVel;
        float forwardVel;
        float yVel;
        float spin;
        float scale;
        bool alive;
    };

    // Tunable parameters (copied from original DisplayIntro2)
    const int kCubeCount = 128;
    const float kSpawnRadiusMin = .7f;
    const float kSpawnRadiusMax = .9f;
    const float kStartZ = 1.5f - .3f; // will be adjusted using supplied zoffset if different
    const float kDisappearZ = 1.5f + 1.5f; // uses supplied zoffset
    const float kMinAngularVel = 1.2f;
    const float kMaxAngularVel = 4.2f;
    const float kMinRadialVel = -0.05f;
    const float kMaxRadialVel = -0.01f;
    const float kMinForwardVel = 0.45f;
    const float kMaxForwardVel = 1.1f;
    const float kMinYVel = 0.06f;
    const float kMaxYVel = 0.18f;
    const float kBaseScale = 5.0f;

    std::vector<SpiralCube> m_cubes;
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_ur01;
    std::uniform_real_distribution<float> m_urNeg1_1;
    float m_lastT;
    bool m_initialized;

    GeometricPrimitive* m_cube;
    GeometricPrimitive* m_teapot;
    GeometricPrimitive* m_dodec;
    FlappyData* m_flappy;
    float m_zoffset;
};