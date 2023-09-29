// Description:
//   Collection of game state variables.
//
// Copyright (C) 2003 Frank Becker
//

#include "GameState.hpp"

char* GameState::licData = 0;
int GameState::licLength = 0;

bool GameState::isDeveloper = false;
bool GameState::isAlive = true;
bool GameState::requestExit = false;

bool GameState::showFPS = false;

double GameState::startOfStep = 0;
float GameState::frameFractionOther = 0.0;
double GameState::startOfGameStep = 0;
float GameState::frameFraction = 0.0;
unsigned int GameState::gameTick = 0;

PausableTimer GameState::mainTimer;
PausableTimer GameState::stopwatch;
R250 GameState::r250;
Context::ContextEnum GameState::context = Context::eUnknown;
double GameState::secondsPlayed = 0;

float GameState::prevShaftPitch = 0.0;
float GameState::prevShaftYaw = 0.0;
float GameState::shaftPitch = 0.0;
float GameState::shaftYaw = 0.0;

std::string GameState::deviceId("Desktop");
bool GameState::isPirate = false;
