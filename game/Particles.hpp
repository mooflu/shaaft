#pragma once
// Description:
//   All kinds of particles.
//
// Copyright (C) 2008 Frank Becker
//
#include <string>

#include <Point.hpp>
#include <GLBitmapFont.hpp>
#include <ParticleType.hpp>

class Particles
{
public:
    static void Initialize( void);
};

class BitmapParticleType: public ParticleType
{
public:
    BitmapParticleType( const std::string &name);
    virtual ~BitmapParticleType();

    virtual void init( ParticleInfo *p) = 0;
    virtual bool update( ParticleInfo *p) = 0;
    virtual void draw( ParticleInfo *p) = 0;

    static void bindTexture( void) { _bitmaps->bind();}

protected:
    static GLBitmapCollection *_bitmaps;
    void LoadBitmaps( void);
};

class SingleBitmapParticle: public BitmapParticleType
{
public:
    SingleBitmapParticle( const std::string &name, const char *bitmapName);

protected:
    float _bmHalfWidth;
    float _bmHalfHeight;
    float _radius;

    int _bmIndex;
};

class Spark: public SingleBitmapParticle
{
public:
    Spark( void);
    virtual ~Spark();

    virtual void init( ParticleInfo *p);
    virtual bool update( ParticleInfo *p);
    virtual void draw( ParticleInfo *p);

private:
};

class StatusMessage: public ParticleType
{
public:
    StatusMessage( void);
    virtual ~StatusMessage();

    virtual void init( ParticleInfo *p);
    virtual bool update( ParticleInfo *p);
    virtual void draw( ParticleInfo *p);

protected:
    GLBitmapFont *_smallFont;
};

class ScoreHighlight: public ParticleType
{
public:
    ScoreHighlight( void);
    virtual ~ScoreHighlight();

    virtual void init( ParticleInfo *p);
    virtual bool update( ParticleInfo *p);
    virtual void draw( ParticleInfo *p);

protected:
    vec3 _color;
    std::string _value;
    GLBitmapFont *_font;
};
