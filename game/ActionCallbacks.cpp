// Description:
//   Action callbacks for mouse and keyboard events.
//
// Copyright (C) 2007 Frank Becker
//
#include "ActionCallbacks.hpp"

#include "Trace.hpp"
#include "Trigger.hpp"
#include "MenuManager.hpp"
#include "VideoBase.hpp"

using namespace std;

void MotionAction::performAction( Trigger &trigger, bool /*isDown*/)
{
//    XTRACE();
    switch( GameState::context)
    {
#ifndef IPHONE            
        case Context::eInGame:
            GameState::shaftPitch += (float)(trigger.fData1)/3.0;
            Clamp( GameState::shaftPitch, -40.0, 40.0);

            GameState::shaftYaw -= (float)(trigger.fData2)/3.0;
            Clamp( GameState::shaftYaw, -40.0, 40.0);
            break;
#endif            
        default:
            break;
    }
}

void SnapshotAction::performAction( Trigger &, bool isDown)
{
//    XTRACE();
    if( !isDown) return;

    VideoBaseS::instance()->takeSnapshot();
}

void ConfirmAction::performAction( Trigger &, bool isDown)
{
//    XTRACE();
    if( !isDown) return;

    LOG_INFO << "Yes Sir!" << endl;

#if 0
    switch( GameState::context)
    {
	default:
	    break;
    }
#endif
}

void PauseGame::performAction( Trigger &, bool isDown)
{
//    XTRACE();
    if( !isDown) return;

    if( GameState::context == Context::ePaused)
    {
	LOG_INFO << "un-pausing..." << endl;
	GameState::context = _prevContext;
	GameState::stopwatch.start();
    }
    else
    {
	LOG_INFO << "pausing..." << endl;
	_prevContext = GameState::context;
	GameState::context = Context::ePaused;
	GameState::stopwatch.pause();
    }
}

void EscapeAction::performAction( Trigger &, bool isDown)
{
//    XTRACE();
    if( !isDown) return;

    switch( GameState::context)
    {
	case Context::eMenu:
	    break;

	default:
//	    LOG_INFO << "Menu mode..." << endl;
            MenuManagerS::instance()->turnMenuOn();
	    break;
    }
}
