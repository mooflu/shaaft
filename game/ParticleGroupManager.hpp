#pragma once
// Description:
//   Particle group manager.
//
// Copyright (C) 2008 Frank Becker
//
#include <string>
#include <list>
#include <hashMap.hpp>

#include <HashString.hpp>
#include <Singleton.hpp>

class ParticleGroup;

class ParticleGroupManager
{
friend class Singleton<ParticleGroupManager>;
public:
    bool init( void);
    void reset( void);
    void draw( void);
    bool update( void);

    void addGroup( const std::string &groupName, int groupSize);
    void addLink( const std::string &group1, const std::string &group2);
    ParticleGroup *getParticleGroup( const std::string &groupName);

    int getAliveCount( void);

private:
    ~ParticleGroupManager();
    ParticleGroupManager( void);

    ParticleGroupManager( const ParticleGroupManager&);
    ParticleGroupManager &operator=(const ParticleGroupManager&);

    hash_map< 
        const std::string, ParticleGroup*, hash<const std::string>, std::equal_to<const std::string> > _particleGroupMap;
    std::list<ParticleGroup*> _particleGroupList;

    struct LinkedParticleGroup
    {
        ParticleGroup *group1;
        ParticleGroup *group2;
    }; 

    std::list<LinkedParticleGroup*> _linkedParticleGroupList;
};

typedef Singleton<ParticleGroupManager> ParticleGroupManagerS;
