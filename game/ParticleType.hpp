#pragma once
// Description:
//   Particle type base.
//
// Copyright (C) 2008 Frank Becker
//
#include <string>

#include <Point.hpp>
#include <ParticleInfo.hpp>

class ParticleType
{
public:
    ParticleType( const std::string &name, bool manage = true);
    virtual ~ParticleType();

    virtual void init( ParticleInfo *p) = 0;
    virtual bool update( ParticleInfo *p) = 0;
    virtual void draw( ParticleInfo *p) = 0;

    virtual void hit( ParticleInfo *p, int /*damage*/, int radIndex=0) { p->tod = 0; radIndex=0;}
    virtual void hit( ParticleInfo *p, ParticleInfo *p2, int radIndex=0) { hit( p, p2->damage, radIndex);}

    virtual int getRadiiCount(void) { return 1;}
    virtual float getRadius(int /*radIndex*/) { return 0.0f;}
    virtual vec3 getOffset(int /*radIndex*/) { vec3 dummy(0,0,0); return dummy;}

    const std::string& name( void){ return _name;}

protected:
    void updatePrevs( ParticleInfo *p);
    void interpolate( ParticleInfo *p, ParticleInfo &pi);
    void interpolateOther( ParticleInfo *p, ParticleInfo &pi);

private:
    void interpolateImpl( ParticleInfo *p, ParticleInfo &pi, const float &gf);
    const std::string _name;
};
