#pragma once
// Description:
//   Collection of game state variables.
//
// Copyright (C) 2003 Frank Becker
//

#include "Context.hpp"
#include "PausableTimer.hpp"
#include "R250.hpp"

#include <string>

struct GameState
{
    static char *licData;
    static int  licLength;
    
    static bool isDeveloper;
    static bool isAlive;
    static bool requestExit;

    static bool showFPS;

    static float startOfStep;
    static float frameFractionOther;
    static float startOfGameStep;
    static float frameFraction;
    static unsigned int gameTick;

    static PausableTimer mainTimer;
    static PausableTimer stopwatch;
    static R250 r250;
    static Context::ContextEnum context;
    static float secondsPlayed;
    
    static float prevShaftPitch;
    static float prevShaftYaw;
    static float shaftPitch;
    static float shaftYaw;
    static std::string deviceId;
    static bool isPirate;
};
