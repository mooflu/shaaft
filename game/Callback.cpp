// Description:
//   Base for callbacks.
//
// Copyright (C) 2007 Frank Becker
//
#include "Callback.hpp"

#include "Trace.hpp"
#include "Input.hpp"

using namespace std;

Callback::Callback( const string &actionName, const string &defaultTrigger):
    _actionName( actionName),
    _defaultTrigger( defaultTrigger)
{
    XTRACE();
    InputS::instance()->addCallback( this);
}

Callback::~Callback()
{
    XTRACE();
}
