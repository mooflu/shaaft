#pragma once
// Description:
//   Input subsystem.
//
// Copyright (C) 2007 Frank Becker
//
#include <string>

#include "Trace.hpp"
#include "hashMap.hpp"
#include "Singleton.hpp"
#include "Trigger.hpp"
#include "Keys.hpp"
#include "ConfigHandler.hpp"
#include "CallbackManager.hpp"
#include "InterceptorI.hpp"

#include <vmmlib/vector.hpp>
using namespace vmml;

class Callback;

#if defined(EMSCRIPTEN)
#define ESCAPE_KEY SDLK_BACKSPACE
#else
#define ESCAPE_KEY SDLK_ESCAPE
#endif

namespace HASH_NAMESPACE
{
    template<>
	struct hash<Trigger>
    {
	//a simple hash function for Trigger
	int operator()(const Trigger &t) const
	{
	    int hashval;

	    if( t.type == eMotionTrigger)
	    {
		hashval = t.type*1000;
	    }
	    else
	    {
		hashval = t.type*1000+t.data1;
	    }

	    return hashval;
	}
    };
}

struct TouchInfo
{
    TouchInfo( void *t, const vec2i &p):
        touch(t),
        pos(p),
        currentPos(p),
        active(true)
    {
    }

    void *touch;
    vec2i pos;
    vec2i currentPos;
    bool active;
};

class Input: public ConfigHandler
{
friend class Singleton<Input>;
public:
    bool init( void);
    bool update( void);

    //used for loading/saving bindings
    virtual void handleLine( const std::string line);
    virtual void save( std::ostream &of);

    //Input takes ownership of callback
    void bindNextTrigger( const std::string &action)
    {
        _bindMode = true;
        _action = action;
        LOG_INFO << "bindNextTrigger -> " << _action << "\n";
    }

    bool waitingForBind( void)
    {
        return _bindMode;
    }

    void addCallback( Callback *cb);

    std::string getTriggerName( std::string &action);

    void enableInterceptor( InterceptorI *i)
    {
	_interceptor = i;
    }

    void disableInterceptor( void)
    {
	_interceptor = 0;
    }

    const vec2f& mousePos(void);
    void resetMousePosition();

    std::vector<TouchInfo*> getActiveTouches();

private:
    virtual ~Input();
    Input( void);
    Input( const Input&);
    Input &operator=(const Input&);

    void bind( Trigger &t, Callback *action);
    bool tryGetTrigger( Trigger &trigger, bool &isDown);
    void updateMouseSettings( void);

#ifdef IPHONE
    void handleTouch( SDL_TouchEvent &touch);
#endif

    bool _bindMode;
    std::string _action;
    CallbackManager _callbackManager;
    Keys _keys;

	typedef  hash_map< std::string, Trigger*, hash<std::string>, std::equal_to<std::string> > ATMap;
    ATMap _actionTriggerMap;

	hash_map< Trigger, Callback*, hash<Trigger>, std::equal_to<Trigger> > _callbackMap;

    //mouse position [0..1]
    vec2f _mousePos;
    vec2f _mouseDelta;

    //intercept raw input
    InterceptorI *_interceptor;

    //Touch information
    int _touchCount;
    std::vector<TouchInfo> _touches;

#ifdef IPHONE
    int addTouch( void *t, const vec2i &p);
    void removeTouch( int button);
    int removeTouch( void *t);
    int findTouch( void *t, const vec2i &currentPos);
#endif
};

typedef Singleton<Input> InputS;
