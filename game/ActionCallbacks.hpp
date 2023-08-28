#pragma once
// Description:
//   Action callbacks for mouse and keyboard events.
//
// Copyright (C) 2007 Frank Becker
//

#include "Trace.hpp"
#include "Callback.hpp"
#include "GameState.hpp"

class MotionAction: public Callback
{
public:
    MotionAction( void): Callback( "Motion", "MOTION") { XTRACE(); }
    virtual ~MotionAction() { XTRACE(); }
    virtual void performAction( Trigger &trigger, bool isDown);
};

class SnapshotAction: public Callback
{
public:
    SnapshotAction( void): Callback( "Snapshot", "F6") { XTRACE(); }
    virtual ~SnapshotAction() { XTRACE(); }
    virtual void performAction( Trigger &trigger, bool isDown);
};

class ConfirmAction: public Callback
{
public:
    ConfirmAction( void): Callback( "Confirm", "RETURN") { XTRACE(); }
    virtual ~ConfirmAction() { XTRACE(); }
    virtual void performAction( Trigger &, bool isDown);
};

class PauseGame: public Callback
{
public:
    PauseGame( void): Callback( "PauseGame", "P"),
        _prevContext( Context::eUnknown)
    {
        XTRACE();
    }
    virtual ~PauseGame() { XTRACE(); }
    virtual void performAction( Trigger &, bool isDown);
private:
    Context::ContextEnum _prevContext;
};

class EscapeAction: public Callback
{
public:
    EscapeAction( void): Callback( "EscapeAction", "ESCAPE"),
        _prevContext( Context::eUnknown)
    {
        XTRACE();
    }
    virtual ~EscapeAction() { XTRACE(); }
    virtual void performAction( Trigger &, bool isDown);
private:
    Context::ContextEnum _prevContext;
};
