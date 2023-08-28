#pragma once
// Description:
//   Block View
//
// Copyright (C) 2007 Frank Becker
//

#include "Quaternion.hpp"
#include "GLBitmapFont.hpp"
#include "Model.hpp"

#include "BlockModel.hpp"
#include "VideoBase.hpp"
#include "TextInput.hpp"

class Buffer;
class VertexArray;

class BlockView: public ResolutionChangeObserverI
{
public:
    BlockView( BlockModel &model);
    virtual ~BlockView();

    bool init( void);
    void draw( void);

    virtual void resolutionChanged( int w, int h);

    // game step update
    void update( void);
    void updateSettings( void);

    //The model will call these when a new block or a new
    //rotation was applied.
    void notifyNewBlock( void);
    void notifyNewRotation( const Quaternion &);

    //Get interpolated values based on the current game state frameFraction
    Point3D &getInterpolatedOffset( void);
    float getInterpolatedAngle( void);

    Point3D &getCurrentAxis( void) { return _currentAxis; }
    float getPrevAngle( void) { return _prevAngle; }
    Point3D &getPrevAxis( void) { return _prevAxis; }

private:
    BlockView( const BlockView&);
    BlockView &operator=(const BlockView&);

    void initGL3Test();

    enum BlockType
    {
        Normal,
        Locked,
        Hint,
        Lookahead,
    };
    void drawElement( Point3Di *p, BlockType blockType);
    void drawLockedElements( void);

    void drawShaft( bool drawLines);
    void drawIndicator( void);
    void drawNextBlock( void);

    vec4f getColor( int p);

    BlockModel &_model;

    float _squaresize;
    float _bottom;

    bool _useAALines;
    bool _showNextBlock;
    bool _showBlockIndicator;

    float _blockAngle;

    GLBitmapFont *_font;
    GLBitmapFont *_fineFont;
    GLTexture *_blockFace;

    void resetRotations( void);

    float _rotationSpeed;
    int _moveSteps;

    Point3D _interpOffset;
    Point3Di _oldOffset;
    Point3D _delta;
    Point3D _targetOffset;
    Point3D _currentOffset;
    Point3D _prevOffset;
    int _numSteps;
    float _prev2Angle;

    Quaternion _prevQ;
    Point3D _prevAxis;
    float _prevAngle;

    Quaternion _targetQ;
    Point3D _currentAxis;
    float _currentAngle;
    float _targetAngle;

    TextInput _textInput;

    Model *_cube;
    Model *_indicator;
    int _blackBox;
    int _pausedBox;
    int _scoreBoard;
    int _moo;
    int _mooChoo;
    int _target;

    Buffer *_shaftVerts;
    Buffer *_shaftNormals;
    Buffer *_shaftVindices;

    VertexArray *_shaftVao;
};
