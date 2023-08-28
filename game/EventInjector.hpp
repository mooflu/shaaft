#pragma once
// Description:
//   Creates SDL events from stream (playback)
//
// Copyright (C) 2003 Frank Becker
//

#include "SDL.h"

#include <string>
#include <iostream>

#include "Trace.hpp"
#include "Tokenizer.hpp"
#include "GameState.hpp"

class EventInjector
{
public:
    EventInjector( std::istream &in):
	_inStream(in)
    {
    	getNextEvent();
    }

    void getNextEvent( void)
    {
	if( _inStream.eof())
	    _nextGameTick = (unsigned int)-1;
	else
	{
	    std::string line;
	    getline( _inStream, line);

	    Tokenizer token(line);
	    _nextGameTick = atoi( token.next().c_str());
	    _nextEvent.type = atoi( token.next().c_str());
//	    LOG_INFO << "Event type = " << (unsigned int)_nextEvent.type << "\n";

	    switch( _nextEvent.type)
	    {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		    _nextEvent.key.keysym.sym = (SDL_Keycode)atoi( token.next().c_str()); // TODO: SDL1 - probably broken
//		    LOG_INFO << "Key = " << (unsigned int)_nextEvent.key.keysym.sym << "\n";
		    break;
	    }
	}

//	LOG_INFO << "Next tick = " << _nextGameTick << "\n";
    }

    bool getEvent( SDL_Event &event)
    {
	if( _nextGameTick <= GameState::gameTick)
	{
	    event = _nextEvent;
	    getNextEvent();
	    return true;
	}
	return false;
    }

private:
    std::istream &_inStream;
    unsigned int _nextGameTick; 
    SDL_Event _nextEvent;
};
