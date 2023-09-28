#pragma once
// Description:
//   Structure to keep particle information.
//
// Copyright (C) 2008 Frank Becker
//
#include <string>
#include <Point.hpp>

class ParticleType;

struct ParticleInfo {
    //values for current game step position
    vec3 position;
    vec3 velocity;
    vec3 color;
    vec3 extra;

    //previous game step values for interpolation
    vec3 prevPosition;
    vec3 prevVelocity;
    vec3 prevColor;
    vec3 prevExtra;

    Point3D points[4];  //E.g.: Bezier curve data

    float tod;     //time of death
    float radius;  //radius for collision detection

    int damage;  //damage the particle inflicts

    std::string text;  //some text associated with the particle

    ParticleInfo* next;
    ParticleType* particle;

    ParticleInfo* related;  //used for swarm leader
};
