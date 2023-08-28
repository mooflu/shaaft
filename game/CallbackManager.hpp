#pragma once
// Description:
//   Callback manager.
//
// Copyright (C) 2007 Frank Becker
//

#include <string>

#include "hashMap.hpp"
#include "HashString.hpp"

class Callback;

class CallbackManager
{
public:
    CallbackManager( void);
    ~CallbackManager();
    void init( void);

    Callback *getCallback( std::string actionString);
    void addCallback( Callback *cb);

private:
    CallbackManager( const CallbackManager&);
    CallbackManager &operator=(const CallbackManager&);

	hash_map< std::string, Callback*, hash<std::string>, std::equal_to<std::string> > _actionMap;
};
