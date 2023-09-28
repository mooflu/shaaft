#pragma once
// Description:
//   Converts SDL events to stream
//
// Copyright (C) 2003 Frank Becker
//

#include "SDL.h"

#include <iostream>

#include "GameState.hpp"

class EventWatcher {
public:
    EventWatcher(std::ostream& os) :
        _outStream(os) {}

    void notify(SDL_Event& event) {
        switch (event.type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                _outStream << GameState::gameTick << " ";
                _outStream << (unsigned int)event.type << " ";
                _outStream << (unsigned int)event.key.keysym.sym << "\n";
                break;
        }
    }

private:
    std::ostream& _outStream;
};
