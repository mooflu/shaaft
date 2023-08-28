// Description:
//   Particle type base.
//
// Copyright (C) 2008 Frank Becker
//
#include "ParticleType.hpp"

#include <Trace.hpp>
#include <ParticleGroup.hpp>
#include <GameState.hpp>

using namespace std;

ParticleType::ParticleType( const string &particleName, bool manage):
    _name( particleName)
{
    XTRACE();
    if( manage) ParticleGroup::addParticleType( this);
}

ParticleType::~ParticleType()
{
    XTRACE();
}

void ParticleType::updatePrevs( ParticleInfo *p)
{
    p->prevPosition = p->position;
    p->prevVelocity = p->velocity;
    p->prevExtra = p->extra;
    p->prevColor = p->color;
}

void ParticleType::interpolate( ParticleInfo *p, ParticleInfo &pi)
{
    interpolateImpl( p, pi, GameState::frameFraction);
}

void ParticleType::interpolateOther( ParticleInfo *p, ParticleInfo &pi)
{
    interpolateImpl( p, pi, GameState::frameFractionOther);
}

void ParticleType::interpolateImpl( ParticleInfo *p, ParticleInfo &pi, const float &gf)
{
    pi.position.x = p->prevPosition.x+ (p->position.x - p->prevPosition.x) * gf;
    pi.position.y = p->prevPosition.y+ (p->position.y - p->prevPosition.y) * gf;
    pi.position.z = p->prevPosition.z+ (p->position.z - p->prevPosition.z) * gf;

#if 0
//velocity and color interpolation isn't required for now.

    pi.velocity.x = p->prevVelocity.x+ (p->velocity.x - p->prevVelocity.x) * gf;
    pi.velocity.y = p->prevVelocity.y+ (p->velocity.y - p->prevVelocity.y) * gf;
    pi.velocity.z = p->prevVelocity.z+ (p->velocity.z - p->prevVelocity.z) * gf;
#endif

    pi.extra.x = p->prevExtra.x + (p->extra.x - p->prevExtra.x) * gf;
    pi.extra.y = p->prevExtra.y + (p->extra.y - p->prevExtra.y) * gf;
    pi.extra.z = p->prevExtra.z + (p->extra.z - p->prevExtra.z) * gf;
}
