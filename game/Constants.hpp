#pragma once
// Description:
//   All kinds of constants (record)
//
// Copyright (C) 2003 Frank Becker
//

#include <string>

#ifdef HAVE_CONFIG_H
#include <defines.h>  //PACKAGE and VERSION
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

#if 0
template <typename TYPE>
inline TYPE MIN( const TYPE v1, const TYPE v2)
{
    return (v1<v2?v1:v2);
}

template <typename TYPE>
inline TYPE MAX( const TYPE v1, const TYPE v2)
{
    return (v1>v2?v1:v2);
}
#endif

const std::string GAMETITLE = PACKAGE;
const std::string GAMEVERSION = VERSION;

const std::string EFFECTS_GROUP1 = "Effects1";
const std::string EFFECTS_GROUP2 = "Effects2";
const std::string EFFECTS_GROUP3 = "Effects3";

const int MAX_PARTICLES_PER_GROUP = 2048;

const float GAME_STEP_SIZE = (float)(1.0 / 30.0);  //run logic 30 times per second
const int MAX_GAME_STEPS = 20;                     //max number of logic runs per frame

// All updates in out logic are based on a game step size of 1/50.
// In case we want to use a different GAME_STEP_SIZE in the future,
// multiply all update values by GAME_STEP_SCALE.
const float GAME_STEP_SCALE = (float)(50.0 * GAME_STEP_SIZE);
