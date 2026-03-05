
#include <Windows.h>
#include <d3d11.h>
#include <d2d1.h>
#include <DirectXTex.h>
#include <DirectXMath.h>
#include <GeometricPrimitive.h>

#include "IntroScene.h"
#include "SimpleMath.h"
#include "VertexTypes.h"
#include <cmath>

using namespace DirectX;

IntroScene::IntroScene(GeometricPrimitive* cube,
                       GeometricPrimitive* teapot,
                       GeometricPrimitive* dodec,
                       FlappyData* flappyData,
                       float zoffset)
    : m_cube(cube)
    , m_teapot(teapot)
    , m_dodec(dodec)
    , m_flappy(flappyData)
    , m_zoffset(zoffset)
    , m_rng(static_cast<unsigned>(time(nullptr)))
    , m_ur01(0.f, 1.f)
    , m_urNeg1_1(-1.f, 1.f)
    , m_initialized(false)
    , m_explosionInitialized(false)
    , m_lastT(0.f)
{
    // lazy initialization on first Draw call
}

void IntroScene::Reset()
{
    m_initialized = false;
    m_explosionInitialized = false;
    m_lastT = 0.f;
    m_cubes.clear();
}

void IntroScene::Draw(float t, const XMMATRIX& g_World, const XMMATRIX& g_View, const XMMATRIX& g_Projection)
{
    // Initialize cubes once
    if (!m_initialized)
    {
        m_cubes.clear();
        m_cubes.reserve(kRows * kCols);
        for (int r = 0; r < kRows; ++r)
        {
            for (int c = 0; c < kCols; ++c)
            {
                CubeState cs{};
                float cx = (c - (kCols - 1) * 0.5f) * kSpacing;
                float cy = ((kRows - 1) * 0.5f - r) * kSpacing;
                cs.pos = XMFLOAT3(cx, cy, m_zoffset - 0.6f);
                cs.vel = XMFLOAT3(0.f, 0.f, 0.f);
                cs.ang = XMFLOAT3(m_urNeg1_1(m_rng) * 0.05f, m_urNeg1_1(m_rng) * 0.05f, m_urNeg1_1(m_rng) * 0.05f);
                cs.angVel = XMFLOAT3(0.f, 0.f, 0.f);
                cs.scale = kCubeBaseScale * (0.9f + 0.2f * m_ur01(m_rng));
                m_cubes.push_back(cs);
            }
        }
        m_initialized = true;
        m_lastT = t;
    }

    // frame dt
    float dt = std::abs((m_lastT == 0.f) ? 0.f : (t - m_lastT));
    if (dt < 0.f) dt = 0.f;
    m_lastT = t;

    // Draw intact wall before breakStart; add idle wobble.
    if (t > kBreakStart)
    {
        float wobble = sinf(t * 3.5f) * 0.03f;
        int idx = 0;
        for (int r = 0; r < kRows; ++r)
        {
            for (int c = 0; c < kCols; ++c)
            {
                auto& cs = m_cubes[idx++];
                XMFLOAT3 translate = cs.pos;
                translate.y += wobble * (1.f + 0.2f * ((r + c) % 3));
                translate.x += 0.02f * sinf(t * (0.6f + 0.1f * idx) + idx);
                XMVECTOR Scaling = XMVectorSet(cs.scale, cs.scale, cs.scale, 0.f);
                XMVECTOR rotateQ = XMQuaternionRotationRollPitchYaw(cs.ang.x, cs.ang.y, cs.ang.z);
                auto local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorZero(), rotateQ,
                    XMVectorSet(translate.x, translate.y, translate.z, 0)));
                if (m_cube) m_cube->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
            }
        }

        // small accent teapot/dodec
        if (m_teapot)
        {
            XMFLOAT3 translate(-0.9f, -.25f, m_zoffset - 0.1f);
            XMVECTOR Scaling = XMVectorSet(.3f, .3f, .3f, .3f);
            XMVECTOR rotateQ = XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(0.f, m_flappy ? m_flappy->animateT : t, 0.f, 0.f));
            auto local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorZero(), rotateQ,
                XMVectorSet(translate.x, translate.y, translate.z, 0)));
            m_teapot->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
        }
        if (m_dodec)
        {
            XMFLOAT3 translate(.6f, -.3f, m_zoffset - 0.1f);
            XMVECTOR Scaling = XMVectorSet(.15f, .15f, .15f, .15f);
            XMVECTOR rotateQ = XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(0.f, - (m_flappy ? m_flappy->animateT : t) * 8.f, 0.f, 0.f));
            auto local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorZero(), rotateQ,
                XMVectorSet(translate.x, translate.y, translate.z, 0)));
            m_dodec->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
        }
        return;
    }

    // After break start, seed explosion velocities once.
    if (!m_explosionInitialized)
    {
        m_explosionInitialized = true;
        XMFLOAT3 center{ 0.f, 0.f, m_zoffset - 0.6f };
        for (auto& cs : m_cubes)
        {
            XMVECTOR dir = XMVectorSet(cs.pos.x - center.x + m_urNeg1_1(m_rng) * 0.15f,
                                       cs.pos.y - center.y + m_urNeg1_1(m_rng) * 0.15f,
                                       0.f, 0.f);
            XMVECTOR nd = XMVector3Normalize(dir);
            XMFLOAT3 ndF;
            XMStoreFloat3(&ndF, nd);
            float speed = kExplosionSpeedMin + m_ur01(m_rng) * (kExplosionSpeedMax - kExplosionSpeedMin);
            cs.vel.x = ndF.x * speed;
            cs.vel.y = ndF.y * speed * 0.5f + m_urNeg1_1(m_rng) * 0.2f;
            cs.vel.z = (1.2f + m_ur01(m_rng) * 1.6f);
            cs.angVel.x = m_urNeg1_1(m_rng) * kAngularSpeedMax;
            cs.angVel.y = m_urNeg1_1(m_rng) * kAngularSpeedMax;
            cs.angVel.z = m_urNeg1_1(m_rng) * kAngularSpeedMax;
        }
    }

    // Integrate and draw cubes
    for (auto& cs : m_cubes)
    {
        cs.pos.x += cs.vel.x * dt;
        cs.pos.y += cs.vel.y * dt;
        cs.pos.z += cs.vel.z * dt;

        cs.vel.x *= powf(kDamping, dt * 60.f);
        cs.vel.y *= powf(kDamping, dt * 60.f);
        cs.vel.z *= powf(kDamping, dt * 60.f);

        cs.ang.x += cs.angVel.x * dt;
        cs.ang.y += cs.angVel.y * dt;
        cs.ang.z += cs.angVel.z * dt;

        float depthScale = 1.0f + ((m_zoffset - 0.6f) - cs.pos.z) * 0.06f;
        float drawScale = cs.scale * depthScale;

        XMVECTOR Scaling = XMVectorSet(drawScale, drawScale, drawScale, 0.f);
        XMVECTOR rotateQ = XMQuaternionRotationRollPitchYaw(cs.ang.x, cs.ang.y, cs.ang.z);
        auto local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorZero(), rotateQ,
            XMVectorSet(cs.pos.x, cs.pos.y, cs.pos.z, 0)));
        if (m_cube) m_cube->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
    }

    // celebratory teapot/dodec while cubes fly
    if (m_teapot)
    {
        XMFLOAT3 translate(-0.9f + 0.1f * sinf(t * 2.1f), -.25f + 0.08f * cosf(t * 2.3f), m_zoffset - 0.1f);
        XMVECTOR Scaling = XMVectorSet(.28f, .28f, .28f, .28f);
        XMVECTOR rotateQ = XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(0.f, t * 0.9f, 0.f, 0.f));
        auto local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorZero(), rotateQ,
            XMVectorSet(translate.x, translate.y, translate.z, 0)));
        m_teapot->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
    }
    if (m_dodec)
    {
        XMFLOAT3 translate(.6f + 0.06f * cosf(t * 2.9f), -.3f + 0.06f * sinf(t * 3.1f), m_zoffset - 0.1f);
        XMVECTOR Scaling = XMVectorSet(.14f, .14f, .14f, .14f);
        XMVECTOR rotateQ = XMQuaternionRotationRollPitchYawFromVector(XMVectorSet(0.f, -t * 6.f, 0.f, 0.f));
        auto local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorZero(), rotateQ,
            XMVectorSet(translate.x, translate.y, translate.z, 0)));
        m_dodec->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
    }
}