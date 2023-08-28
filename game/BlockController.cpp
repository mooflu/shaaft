// Description:
//   Controller
//
// Copyright (C) 2003 Frank Becker
//
#include "BlockController.hpp"

#include "SDL.h"

#include "Trace.hpp"
#include "Point.hpp"

#include "BlockModel.hpp"
#include "Callback.hpp"
#include "Trigger.hpp"

using namespace std;

const Point3Di px( 1, 0, 0);
const Point3Di mx(-1, 0, 0);
const Point3Di py( 0, 1, 0);
const Point3Di my( 0,-1, 0);
const Point3Di pz( 0, 0, 1);
const Point3Di mz( 0, 0,-1);

class MoveAction: public Callback
{
public:
    MoveAction( BlockModel &m, const string &name, const string &triggerName, 
                BlockModel::Direction dir): 
        Callback( name, triggerName), _direction(dir), _model(m) { XTRACE(); }
    virtual ~MoveAction() { XTRACE(); }
    virtual void performAction( Trigger &, bool isDown)
    {
	if( isDown)
	{
	    _model.moveBlock( _direction);
	}
    }
private:
    BlockModel::Direction _direction;
    BlockModel &_model;
};

class RotateAction: public Callback
{
public:
    RotateAction( BlockModel &m, const string &name, const string &triggerName,
    	const Point3Di& r1, const Point3Di& r2, const Point3Di& r3, const Point3Di& axis): 
        Callback( name, triggerName), 
	_r1(r1), 
	_r2(r2), 
	_r3(r3), 
	_axis(axis), 
	_model(m) { XTRACE(); }
    virtual ~RotateAction() { XTRACE(); }
    virtual void performAction( Trigger &, bool isDown)
    {
	if( isDown)
	{
	    _model.rotateBlock( _r1, _r2, _r3, _axis);
	}
    }
private:
    Point3Di _r1;
    Point3Di _r2;
    Point3Di _r3;
    Point3Di _axis;
    BlockModel &_model;
};

BlockController::BlockController( BlockModel &model):
    _model(model)
{ 
    new MoveAction( _model, "MoveLeft" , "LEFT" ,BlockModel::eLeft);
    new MoveAction( _model, "MoveRight", "RIGHT",BlockModel::eRight);
    new MoveAction( _model, "MoveUp"   , "UP"   ,BlockModel::eUp);
    new MoveAction( _model, "MoveDown" , "DOWN" ,BlockModel::eDown);
    new MoveAction( _model, "MoveIn"   , "SPACE",BlockModel::eIn);

    new RotateAction( _model, "RotateQ", "Q", px, mz, py, px);
    new RotateAction( _model, "RotateA", "A", px, pz, my, mx);
    new RotateAction( _model, "RotateW", "W", pz, py, mx, py);
    new RotateAction( _model, "RotateS", "S", mz, py, px, my);
    new RotateAction( _model, "RotateE", "E", my, px, pz, pz);
    new RotateAction( _model, "RotateD", "D", py, mx, pz, mz);
}

BlockController::~BlockController()
{
}
