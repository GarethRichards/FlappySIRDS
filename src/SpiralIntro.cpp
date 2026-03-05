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
#include "SpiralIntro.h"
#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;

SpiralIntro::SpiralIntro(GeometricPrimitive* cube,
                         GeometricPrimitive* teapot,
                         GeometricPrimitive* dodec,
                         FlappyData* flappyData,
                         float zoffset)
    : m_cube(cube)
    , m_teapot(teapot)
    , m_dodec(dodec)
    , m_flappy(flappyData)
    , m_rng(static_cast<unsigned>(time(nullptr)))
    , m_ur01(0.f, 1.f)
    , m_urNeg1_1(-1.f, 1.f)
    , m_lastT(0.f)
    , m_initialized(false)
    , m_zoffset(zoffset)
{
    // nothing else to do; lazy init on first Draw()
}

void SpiralIntro::Reset()
{
    m_initialized = false;
    m_cubes.clear();
    m_lastT = 0.f;
}

void SpiralIntro::Draw(float t, const XMMATRIX& g_World, const XMMATRIX& g_View, const XMMATRIX& g_Projection)
{
    // compute local start/disappear z based on provided zoffset
    const float startZ = m_zoffset - 0.3f;
    const float disappearZ = m_zoffset + 1.5f;

    if (!m_initialized)
    {
        m_cubes.clear();
        m_cubes.reserve(kCubeCount);
        for (int i = 0; i < kCubeCount; ++i)
        {
            SpiralCube cs;
            cs.angle = m_ur01(m_rng) * XM_2PI;
            cs.radius = kSpawnRadiusMin + m_ur01(m_rng) * (kSpawnRadiusMax - kSpawnRadiusMin);
            cs.y = 0.7f + m_urNeg1_1(m_rng) * 0.6f;
            cs.z = startZ + m_urNeg1_1(m_rng) * -0.25f;
            cs.angVel = kMinAngularVel + m_ur01(m_rng) * (kMaxAngularVel - kMinAngularVel);
            cs.radialVel = kMinRadialVel + m_ur01(m_rng) * (kMaxRadialVel - kMinRadialVel);
            cs.forwardVel = kMinForwardVel + m_ur01(m_rng) * (kMaxForwardVel - kMinForwardVel);
            cs.yVel = kMinYVel + m_ur01(m_rng) * (kMaxYVel - kMinYVel);
            cs.spin = m_urNeg1_1(m_rng) * 3.0f;
            cs.scale = kBaseScale * (0.85f + 0.3f * m_ur01(m_rng));
            cs.alive = true;
            m_cubes.push_back(cs);
        }
        m_initialized = true;
        m_lastT = t;
    }

    // frame dt
    float dt = std::abs((m_lastT == 0.f) ? 0.f : (t - m_lastT));
    if (dt < 0.f) dt = 0.f;
    m_lastT = t;

    // update & draw cubes
    bool anyAlive = false;
    for (auto &cs : m_cubes)
    {
        if (!cs.alive) continue;
        anyAlive = true;

        cs.angle += cs.angVel * dt;
        cs.radius += cs.radialVel * dt;
        cs.z += cs.forwardVel * dt;
        cs.y -= cs.yVel * dt;
        cs.spin += cs.angVel * 0.25f * dt;

        if (cs.z > disappearZ)
        {
            cs.z = startZ + m_urNeg1_1(m_rng) * -0.25f; 
            cs.angVel = kMinAngularVel + m_ur01(m_rng) * (kMaxAngularVel - kMinAngularVel);
            cs.radialVel = kMinRadialVel + m_ur01(m_rng) * (kMaxRadialVel - kMinRadialVel);
            cs.forwardVel = kMinForwardVel + m_ur01(m_rng) * (kMaxForwardVel - kMinForwardVel);
            cs.yVel = kMinYVel + m_ur01(m_rng) * (kMaxYVel - kMinYVel);
            cs.angle = m_ur01(m_rng) * XM_2PI;
            cs.radius = kSpawnRadiusMin + m_ur01(m_rng) * (kSpawnRadiusMax - kSpawnRadiusMin);
        }

        float x = cosf(cs.angle) * cs.radius;
        float y = sinf(cs.angle) * cs.radius;
        float z = cs.z;

        float depthScale = 1.0f + (startZ - z) * 0.06f;
        float drawScale = cs.scale * depthScale;

        XMVECTOR Scaling = XMVectorSet(drawScale, drawScale, drawScale, 0.f);
        XMVECTOR rotateQ = XMQuaternionRotationRollPitchYaw(cs.spin, cs.spin * 0.6f, cs.spin * 0.4f);
        auto local = XMMatrixMultiply(g_World, XMMatrixAffineTransformation(Scaling, XMVectorZero(), rotateQ,
            XMVectorSet(x, y, z, 0)));
        if (m_cube) m_cube->Draw(local, g_View, g_Projection, Colors::WhiteSmoke, nullptr);
    }

    // respawn after pause if none alive
    static float deadTimer = 0.f;
    if (!anyAlive)
    {
        deadTimer += dt;
        if (deadTimer > 1.2f)
        {
            deadTimer = 0.f;
            for (auto &cs : m_cubes)
            {
                cs.angle = m_ur01(m_rng) * XM_2PI;
                cs.radius = kSpawnRadiusMin + m_ur01(m_rng) * (kSpawnRadiusMax - kSpawnRadiusMin);
                cs.y = 0.7f + m_urNeg1_1(m_rng) * 0.6f;
                cs.z = startZ + m_urNeg1_1(m_rng) * 0.25f;
                cs.angVel = kMinAngularVel + m_ur01(m_rng) * (kMaxAngularVel - kMinAngularVel);
                cs.radialVel = kMinRadialVel + m_ur01(m_rng) * (kMaxRadialVel - kMinRadialVel);
                cs.forwardVel = kMinForwardVel + m_ur01(m_rng) * (kMaxForwardVel - kMinForwardVel);
                cs.yVel = kMinYVel + m_ur01(m_rng) * (kMaxYVel - kMinYVel);
                cs.spin = m_urNeg1_1(m_rng) * 3.0f;
                cs.scale = kBaseScale * (0.85f + 0.3f * m_ur01(m_rng));
                cs.alive = true;
            }
        }
    }

    // optional accents (teapot/dodec) similar to original behavior
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