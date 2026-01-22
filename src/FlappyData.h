#pragma once

#include <cstdint>

class ViewingParameters
{
public:
    // viewing-related parameters
    float viewDistance = 500.f;
    float offsetDistance = 600.f;
    float eyeSeparation = 65.f;
    float pmm = 0.f;
    float zNear = 0.f;
    float zFar = 0.f;
    float offset = 0.3f;
};

class FlappyData
{
public:
    enum class EyeUsed {
        LeftEye,
        RightEye,
        CenterEye
    };

    enum class GameMode {
        Intro,
        Play,
        End
    };

    // Mode and eye
    GameMode mode = GameMode::Intro;
    EyeUsed eye = EyeUsed::LeftEye;

    // viewing parameters (moved to separate class)
    ViewingParameters view;

    float flappyY = 0.f;
    float flappyV = 0.f;
    float flappyX = -.4f;

    float ls = .025f;
    float toffset = 0.f;

    float cylinderDiam = .2f;
    float cylinderHeight = 2.f;
    float flappyDiam = .1f;

    float tEnd = 0.f;
    float animateT = 0.f;
    float gapHeight = .30f;

    int score = 0;
    int highScore = 0;
    int iOffset = 0;

    float gravity = 9.8f;
    float flapEffect = 3.f;

    float tLastClick = 0.f;
    float lastKeyPress = 0.f;
    float lastT = 0.f;

    uint64_t timeStart = 0;
    uint64_t deathStart = 0;
};