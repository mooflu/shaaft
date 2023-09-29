// Description:
//   Input subsystem.
//
// Copyright (C) 2007 Frank Becker
//
#include "Input.hpp"

#include <math.h>
#include "SDL.h"

#include "Trace.hpp"
#include "Config.hpp"
#include "Callback.hpp"
#include "GameState.hpp"
#include "FindHash.hpp"
#include "CallbackManager.hpp"
#include "Tokenizer.hpp"
#include "Value.hpp"
#include "VideoBase.hpp"

#ifdef IPHONE
#include "Audio.hpp"
#endif

#ifdef __APPLE__
#define MAIN_MODIFIER KMOD_LCTRL
#else
#define MAIN_MODIFIER KMOD_CTRL
#endif

inline void Clampf(float& val, const float MINval, float MAXval) {
    if (val < MINval) {
        val = MINval;
    } else if (val > MAXval) {
        val = MAXval;
    }
}

using namespace std;

Input::Input(void) :
    _bindMode(false),
    _action(""),
    _callbackManager(),
    _mousePos(0, 0),
    _mouseDelta(0, 0),
    _interceptor(0),
    _touchCount(0) {
    XTRACE();
}

Input::~Input() {
    XTRACE();
    _callbackMap.clear();

    ATMap::iterator i;
    for (i = _actionTriggerMap.begin(); i != _actionTriggerMap.end(); i++) {
        delete i->second;
    }
}

const vec2f& Input::mousePos(void) {
    return _mousePos;
}

void Input::resetMousePosition() {
    _mousePos = vec2f((float)VideoBaseS::instance()->getWidth(), (float)VideoBaseS::instance()->getHeight()) / 2;
}

bool Input::init(void) {
    XTRACE();
    LOG_INFO << "Initializing Input..." << endl;

    _keys.init();

    vector<Config::ConfigItem> bindList;
    ConfigS::instance()->getList("binds", bindList);

    vector<Config::ConfigItem>::iterator i;
    for (i = bindList.begin(); i != bindList.end(); i++) {
        Config::ConfigItem& bind = (*i);
        string action = bind.key;
        string keyname = bind.value;
#if defined(EMSCRIPTEN)
        if (action == "EscapeAction") {
            keyname = "BACKSPACE";
        }
#endif
        LOG_INFO << "action [" << action << "], "
                 << "keyname [" << keyname << "]" << endl;

        Trigger* trigger = new Trigger;
        if (_keys.convertStringToTrigger(keyname, *trigger)) {
            _actionTriggerMap[action] = trigger;
        }
    }

    updateMouseSettings();

    //SDL_EnableKeyRepeat( 300,200); -- SDL1

    _callbackManager.init();

    LOG_INFO << "Input OK." << endl;
    return true;
}

void Input::updateMouseSettings(void) {}

std::vector<TouchInfo*> Input::getActiveTouches() {
    std::vector<TouchInfo*> results;

    for (size_t i = 0; i < _touches.size(); i++) {
        if (_touches[i].touch) {
            results.push_back(&_touches[i]);
        }
    }

    return results;
}

#ifdef IPHONE
int Input::addTouch(void* t, const vec2i& p) {
    _touchCount++;

    for (int i = 0; i < _touches.size(); i++) {
        if (_touches[i].touch == 0) {
            _touches[i] = TouchInfo(t, p);
            return i + 1;
        }
    }

    TouchInfo ti(t, p);
    _touches.push_back(ti);
    return _touches.size();
}

void Input::removeTouch(int button) {
    if (_touches[button - 1].touch) {
        _touchCount--;
        _touches[button - 1].touch = 0;
    }
}

int Input::removeTouch(void* t) {
    for (int i = 0; i < _touches.size(); i++) {
        if (_touches[i].touch == t) {
            _touchCount--;
            _touches[i].touch = 0;
            return i + 1;
        }
    }
    return 0;
}

int Input::findTouch(void* t, const vec2i& currentPos) {
    for (int i = 0; i < _touches.size(); i++) {
        if (_touches[i].touch == t) {
            _touches[i].currentPos = currentPos;
            return i + 1;
        }
    }
    return 0;
}

enum DragDirection {
    eNone,

    eLeft,
    eRight,
    eUp,
    eDown,

    eDiagonal,  // >= than this is diagonal

    eLeftUp,
    eLeftDown,
    eRightUp,
    eRightDown,
};

bool isDiagonal(const vec2i& d, DragDirection& dir) {
    dir = eNone;

    int dx = abs(d.x());
    int dy = abs(d.y());

    if (dx < 20 && dy < 20) {
        return false;
    }

    bool isDiag = false;

    if (dx > 0 && dy > 0) {
        float diag = (float)dx / float(dy);
        if (diag > 0.5 && diag < 2) {
            isDiag = true;
        }
    }

    if (isDiag) {
        if (d.x() > 0) {
            if (d.y() > 0) {
                dir = eRightDown;
            } else {
                dir = eRightUp;
            }
        } else {
            if (d.y() > 0) {
                dir = eLeftDown;
            } else {
                dir = eLeftUp;
            }
        }
    } else {
        if (dx > dy) {
            if (d.x() > 0) {
                dir = eRight;
            } else {
                dir = eLeft;
            }
        } else if (dy > dx) {
            if (d.y() > 0) {
                dir = eDown;
            } else {
                dir = eUp;
            }
        }
    }

    return isDiag;
}

extern std::list<SDL_Event> gEventList;

static inline void pushKey(const SDLKey& key) {
    SDL_Event sdlEvent;

    sdlEvent.key.keysym.mod = KMOD_NONE;
    sdlEvent.key.keysym.sym = key;
    sdlEvent.type = SDL_KEYDOWN;
    gEventList.push_back(sdlEvent);
    sdlEvent.type = SDL_KEYUP;
    gEventList.push_back(sdlEvent);
}

void Input::handleTouch(SDL_TouchEvent& touch) {
    //TODO: for double touch, update start pos every x msec?
    //so continuous touch can become stationary (vs stay a flick)

    std::vector<TouchInfo*> tVec = getActiveTouches();
    if (tVec.size() == 2) {
        vec2i p1s = tVec[0]->pos;
        vec2i p1e = tVec[0]->currentPos;
        vec2i d1 = p1e - p1s;
        DragDirection dir1;
        bool isDiag1 = isDiagonal(d1, dir1);

        vec2i p2s = tVec[1]->pos;
        vec2i p2e = tVec[1]->currentPos;
        vec2i d2 = p2e - p2s;
        DragDirection dir2;
        bool isDiag2 = isDiagonal(d2, dir2);

        if (!isDiag1 && !isDiag2) {
            if (dir1 == dir2) {
                switch (dir1) {
                    case eLeft:
                        pushKey(SDLK_s);
                        break;

                    case eRight:
                        pushKey(SDLK_w);
                        break;

                    case eUp:
                        pushKey(SDLK_a);
                        break;

                    case eDown:
                        pushKey(SDLK_q);
                        break;

                    default:
                        break;
                }
                //turn both touches inactive, so if they aren't released exactly
                //together, there's no additional single touch behaviour triggered.
                tVec[0]->active = false;
                tVec[1]->active = false;
            }
        }

        if (dir1 == eNone || dir2 == eNone) {
            vec3i c;
            bool rot = false;
            if (dir1 == eNone) {
                vec3i v2 = d2;
                vec3i v3 = (p1s - p2s);
                c = v2.cross(v3);
                rot = true;
            }
            if (dir2 == eNone) {
                vec3i v1 = d1;
                vec3i v3 = (p2s - p1s);
                c = v1.cross(v3);
                rot = true;
            }
            if (rot) {
                if (c.z() < 0) {
                    pushKey(SDLK_e);
                } else {
                    pushKey(SDLK_d);
                }
                //turn both touches inactive, so if they aren't released exactly
                //together, there's no additional single touch behaviour triggered.
                tVec[0]->active = false;
                tVec[1]->active = false;
            }
        }

    } else if (tVec.size() == 1 && tVec[0]->active) {
        vec2i posStart = tVec[0]->pos;

        //CGPoint p = [tVec[0]->touch locationInView:self];
        vec2i pos(touch.x, touch.y);  // p.y, 320-p.x);

        if (pos.x() < 75 && pos.y() < 105) {
            pushKey(ESCAPE_KEY);
        } else if (pos.x() < 75 && pos.y() < 205) {
            pushKey(SDLK_p);
        } else if ((pos.x() < 75 && pos.y() < 305) || (touch.tapCount == 2))  //[tVec[0]->touch tapCount]==2) )
        {
            pushKey(SDLK_SPACE);
        } else if (pos.x() > 380 && pos.x() < 420 && pos.y() > 295 && pos.y() < 320) {
            AudioS::instance()->toggleAudioEnabled();
        } else {
            vec2i d = pos - posStart;
            DragDirection dir;
            isDiagonal(d, dir);

            switch (dir) {
                case eLeft:
                    pushKey(SDLK_LEFT);
                    break;

                case eRight:
                    pushKey(SDLK_RIGHT);
                    break;

                case eUp:
                    pushKey(SDLK_UP);
                    break;

                case eDown:
                    pushKey(SDLK_DOWN);
                    break;

                case eLeftDown:
                    pushKey(SDLK_LEFT);
                    pushKey(SDLK_DOWN);
                    break;

                case eLeftUp:
                    pushKey(SDLK_LEFT);
                    pushKey(SDLK_UP);
                    break;

                case eRightDown:
                    pushKey(SDLK_RIGHT);
                    pushKey(SDLK_DOWN);
                    break;

                case eRightUp:
                    pushKey(SDLK_RIGHT);
                    pushKey(SDLK_UP);
                    break;

                default:
                    break;
            }
        }
    }
}
#endif

// Returns false, if there are no more events available.
bool Input::tryGetTrigger(Trigger& trigger, bool& isDown) {
    //    XTRACE();
    isDown = false;

    SDL_Event event;
    if (!SDL_PollEvent(&event)) {
        return false;
    }

    switch (event.type) {
#ifdef IPHONE
        case SDL_TOUCH:
            switch (event.touch.phase) {
                case SDL_TouchPhaseBegan: {
                    int button = addTouch(event.touch.touchId, vec2i(event.touch.x, event.touch.y));

                    trigger.type = eButtonTrigger;
                    trigger.data1 = button;
                    trigger.data2 = event.touch.x;
                    trigger.data3 = event.touch.y;
                    trigger.fData1 = event.touch.x;
                    trigger.fData2 = event.touch.y;
                    isDown = true;
                } break;

                case SDL_TouchPhaseMoved: {
                    int button = findTouch(event.touch.touchId, vec2i(event.touch.x, event.touch.y));
                    if (button == 1) {
                        trigger.type = eUnknownTrigger;
                        trigger.data2 = event.touch.x;
                        trigger.data3 = event.touch.y;
                        _valDX = event.touch.x;
                        _valDY = event.touch.y;
                    }
                    isDown = true;
                } break;

                case SDL_TouchPhaseEnded: {
                    int button = findTouch(event.touch.touchId, vec2i(event.touch.x, event.touch.y));

                    handleTouch(event.touch);

                    removeTouch(button);

                    trigger.type = eButtonTrigger;
                    trigger.data1 = button;
                    trigger.data2 = event.touch.x;
                    trigger.data3 = event.touch.y;
                    trigger.fData1 = event.touch.x;
                    trigger.fData2 = event.touch.y;
                } break;

                case SDL_TouchPhaseCancelled:
                    /*int button =*/removeTouch(event.touch.touchId);
                    break;

                default:
                    break;
            }
            break;

        case SDL_ACCEL:
            break;
#endif
        case SDL_KEYDOWN:
            isDown = true;
            if ((event.key.keysym.sym == SDLK_BACKQUOTE) && (event.key.keysym.mod & KMOD_SHIFT)) {
                LOG_INFO << "Resolution reset..." << endl;
                Value* w = new Value(800);
                ConfigS::instance()->updateKeyword("width", w);
                Value* h = new Value(600);
                ConfigS::instance()->updateKeyword("height", h);

                trigger.type = eUnknownTrigger;
                break;
            }
            if ((event.key.keysym.sym == SDLK_0) && (event.key.keysym.mod & MAIN_MODIFIER)) {
                GameState::shaftPitch = 0.0;
                GameState::shaftYaw = 0.0;
            }
            if ((event.key.keysym.sym == SDLK_f) && (event.key.keysym.mod & MAIN_MODIFIER)) {
                bool fullscreen = false;
                ConfigS::instance()->getBoolean("fullscreen", fullscreen);

                Value* w = new Value(!fullscreen);
                ConfigS::instance()->updateKeyword("fullscreen", w);
            }

#ifndef NO_QUICK_EXIT
            if ((event.key.keysym.sym == SDLK_BACKQUOTE) ||
                ((event.key.keysym.sym == SDLK_q) && (event.key.keysym.mod & MAIN_MODIFIER))) {
                GameState::requestExit = true;
                LOG_WARNING << "Quick Exit invoked..." << endl;

                trigger.type = eUnknownTrigger;
                break;
            }
#endif
            //fall through

        case SDL_KEYUP:
            trigger.type = eKeyTrigger;
            trigger.data1 = event.key.keysym.sym;
            trigger.data2 = event.key.keysym.mod;
            trigger.data3 = event.key.keysym.scancode;
            break;

        case SDL_MOUSEBUTTONDOWN:
            isDown = true;
            //fall through

        case SDL_MOUSEBUTTONUP:
            trigger.type = eButtonTrigger;
            trigger.data1 = event.button.button;
            trigger.data2 = 0;
            trigger.data3 = 0;
            break;

        case SDL_MOUSEMOTION:
            trigger.type = eMotionTrigger;
            trigger.fData1 = (float)event.motion.xrel;
            trigger.fData2 = (float)-event.motion.yrel;
            break;

        case SDL_MOUSEWHEEL:
            isDown = true;
            trigger.type = eButtonTrigger;
            trigger.data1 = SDL_MOUSEWHEEL;
            trigger.data2 = event.wheel.x;
            trigger.data3 = event.wheel.y;
            break;

        case SDL_TEXTINPUT:
            // LOG_INFO << "SDL_TEXTINPUT\n";
            trigger.type = eTextInputTrigger;
            trigger.text = event.text.text;
            trigger.data1 = -1;
            trigger.data2 = (int)strlen(event.text.text);
            break;

        case SDL_TEXTEDITING:
            // LOG_INFO << "SDL_TEXTEDITING\n";
            trigger.type = eTextInputTrigger;
            trigger.text = event.edit.text;
            trigger.data1 = event.edit.start;
            trigger.data2 = event.edit.length;
            break;

        case SDL_QUIT:
            GameState::requestExit = true;
            break;

        default:
            trigger.type = eUnknownTrigger;
            break;
    }

    return true;
}

bool Input::update(void) {
    //    XTRACE();
    bool isDown;
    Trigger trigger;

    static double nextTime = Timer::getTime() + 0.5;
    double thisTime = Timer::getTime();
    if (thisTime > nextTime) {
        updateMouseSettings();
        nextTime = thisTime + 0.5;
    }

    _mouseDelta = vec2f(0, 0);

    while (tryGetTrigger(trigger, isDown)) {
        if (trigger.type == eUnknownTrigger) {
            //for unkown trigger we don't need to do a lookup
            continue;
        }
#ifndef IPHONE
        if (trigger.type == eMotionTrigger) {
            //LOG_INFO << "dx: " << trigger.fData1 << " dy: " << trigger.fData2 << "\n";
            _mouseDelta += vec2f(trigger.fData1, trigger.fData2);
            continue;
        }
#endif
        //Note: motion triggers can't be bound
        if (_bindMode && isDown && _action.size() && (trigger.type != eMotionTrigger)) {
            bool validBind = true;
            switch (trigger.type) {
                case eKeyTrigger:
                    switch (trigger.data1) {
                        case ESCAPE_KEY:
                            validBind = false;
                            break;
                        default:
                            break;
                    }
                    break;

                case eButtonTrigger:
                    break;

                default:
                    break;
            }
            if (validBind) {
                //LOG_INFO << "Trying to get new bind for " << _action << "\n";

                Callback* cb2 = findHash(trigger, _callbackMap);
                if (cb2) {
                    _actionTriggerMap.erase(cb2->getActionName());
                }

                Trigger* t = findHash(_action, _actionTriggerMap);
                Callback* cb = _callbackManager.getCallback(_action);

                if (t) {
                    //get rid of previous trigger mapping
                    _actionTriggerMap.erase(_action);
                    _callbackMap.erase(*t);
                } else {
                    t = new Trigger;
                }

                //update trigger with new settings
                *t = trigger;
                _actionTriggerMap[_action] = t;
                _callbackMap[*t] = cb;

                ConfigS::instance()->updateKeyword(_action, _keys.convertTriggerToString(*t), "binds");
            }

            //go back to normal mode
            _bindMode = false;
            continue;
        }

        if (_interceptor) {
            //feed trigger to interceptor instead of normal callback mechanism
            _interceptor->input(trigger, isDown);

            continue;
        }

        if (!_bindMode) {
            //find callback for this trigger
            //i.e. the action bound to this key
            Callback* cb = findHash(trigger, _callbackMap);
            if (cb) {
                //LOG_INFO << "Callback for [" << cb->getActionName() << "]" << endl;
                cb->performAction(trigger, isDown);
            }
        } else if (!_action.size()) {
            LOG_ERROR << "Input is in bind mode, but no action" << endl;
            _bindMode = false;
        }
    }

    if ((fabs(_mouseDelta.x()) > 1.0e-10) || (fabs(_mouseDelta.y()) > 1.0e-10)) {
        _mousePos += _mouseDelta;
        Clampf(_mousePos.x(), 0, (float)VideoBaseS::instance()->getWidth());
        Clampf(_mousePos.y(), 0, (float)VideoBaseS::instance()->getHeight());

        trigger.fData1 = _mouseDelta.x();
        trigger.fData2 = _mouseDelta.y();
        if (_interceptor) {
            //feed trigger to interceptor instead of normal callback mechanism
            _interceptor->input(trigger, true);
        } else {
            Callback* cb = findHash(trigger, _callbackMap);
            if (cb) {
                //LOG_INFO << "Callback for [" << cb->getActionName() << "]" << endl;
                cb->performAction(trigger, isDown);
            }
        }
    }

    return true;
}

void Input::handleLine(const string line) {
    //    XTRACE();
    Tokenizer t(line);
    string bindKeyword = t.next();
    if (bindKeyword != "bind") {
        return;
    }

    string action = t.next();
    string keyname = t.next();

#if defined(EMSCRIPTEN)
    if (action == "EscapeAction") {
        keyname = "BACKSPACE";
    }
#endif

    LOG_INFO << "action [" << action << "], "
             << "keyname [" << keyname << "]" << endl;

    Trigger* trigger = new Trigger;
    if (_keys.convertStringToTrigger(keyname, *trigger)) {
        _actionTriggerMap[action] = trigger;
    }
}

void Input::save(ostream& outfile) {
    XTRACE();
    outfile << "# --- Binding section --- " << endl;

    hash_map<Trigger, Callback*, hash<Trigger>>::const_iterator ci;
    for (ci = _callbackMap.begin(); ci != _callbackMap.end(); ci++) {
        outfile << "bind " << ci->second->getActionName() << " " << _keys.convertTriggerToString(ci->first) << endl;
    }
}

void Input::addCallback(Callback* cb) {
    if (cb) {
        _callbackManager.addCallback(cb);

        Trigger* t = findHash(cb->getActionName(), _actionTriggerMap);
        if (!t) {
            LOG_INFO << "trigger not found for " << cb->getActionName() << " - using default\n";
            //add default
            t = new Trigger;
            if (_keys.convertStringToTrigger(cb->getDefaultTriggerName(), *t)) {
                Callback* cbAlreadyThere = findHash(*t, _callbackMap);
                if (cbAlreadyThere) {
                    delete t;
                    return;
                }
                _actionTriggerMap[cb->getActionName()] = t;
            }
        }

        if (t) {
            bind(*t, cb);
        }
    }
}

std::string Input::getTriggerName(std::string& action) {
    Trigger* t = findHash(action, _actionTriggerMap);
    if (t) {
        return _keys.convertTriggerToString(*t);
    }
    return "Not assigned!";
}

void Input::bind(Trigger& trigger, Callback* callback) {
    XTRACE();
    Callback* cb = findHash(trigger, _callbackMap);
    if (cb) {
        LOG_WARNING << "Removing old binding" << endl;
        //remove previous callback...
        _callbackMap.erase(trigger);
    }

    LOG_INFO << "Creating binding for " << callback->getActionName() << " - " << trigger.type << ":" << trigger.data3
             << " sym: " << trigger.data1 << endl;
    _callbackMap[trigger] = callback;
}
