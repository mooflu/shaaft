// Description:
//   Callback manager.
//
// Copyright (C) 2007 Frank Becker
//
#include "CallbackManager.hpp"

#include "Trace.hpp"
#include "ActionCallbacks.hpp"
#include "Callback.hpp"
#include "FindHash.hpp"

using namespace std;

CallbackManager::CallbackManager(void) {
    XTRACE();
}

CallbackManager::~CallbackManager() {
    XTRACE();

    hash_map<string, Callback*, hash<string>>::const_iterator ci;
    for (ci = _actionMap.begin(); ci != _actionMap.end(); ci++) {
        delete ci->second;
    }

    _actionMap.clear();
}

void CallbackManager::init(void) {
    new MotionAction();
    new ConfirmAction();
    new SnapshotAction();
    new PauseGame();
    new EscapeAction();
}

void CallbackManager::addCallback(Callback* cb) {
    XTRACE();
    LOG_INFO << "Adding callback for action [" << cb->getActionName() << "]" << endl;
    _actionMap[cb->getActionName()] = cb;
}

Callback* CallbackManager::getCallback(string actionString) {
    XTRACE();
    Callback* cb = findHash(actionString, _actionMap);
    if (!cb) {
        LOG_ERROR << "Unable to find callback for " << actionString << endl;
        string dummyAction = "TertiaryFire";
        cb = findHash(dummyAction, _actionMap);
    }
    return cb;
}
