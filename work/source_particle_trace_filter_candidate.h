#pragma once
#include <cstdint>
#include "source_sfm_collision_group_candidate.h"

// Snapshot of the values the TF client's standard trace filter reads.
// A null baseEntity means the candidate has no client entity behind it.
struct SourceParticleTraceEntityCandidate {
    uintptr_t handle = 0;
    uintptr_t baseEntity = 0;
    uintptr_t owner = 0;
    int32_t modelType = 0;
    int32_t solidType = 0;
    int32_t moveType = 0;
    int32_t collisionGroup = 0;
    bool transparent = false;
};

// Used for non-static entities. The engine handles static props directly.
// ShouldCollide and game rules are per-scene dependencies. The particle
// query constructs the standard filter with no extra predicate.
template<class EntityRule,class GroupRule>
bool sourceParticleTraceShouldHitEntityCandidate(
    const SourceParticleTraceEntityCandidate& entity,
    const SourceParticleTraceEntityCandidate& excluded,
    uint32_t contentsMask,int32_t particleGroup,
    EntityRule&& entityRule,GroupRule&& groupRule){
    constexpr uint32_t window = 0x2u;
    constexpr uint32_t moveable = 0x4000u;
    constexpr uint32_t monster = 0x02000000u;
    constexpr int32_t brushModel = 1, bspSolid = 1, physicsSolid = 6, pushMove = 7;

    if(!entity.baseEntity)return false;
    const bool brushSolid=entity.modelType==brushModel&&
                          (entity.solidType==bspSolid||entity.solidType==physicsSolid);
    if(!brushSolid&&!(contentsMask&monster))return false;
    if(entity.transparent&&!(contentsMask&window))return false;
    if(entity.moveType==pushMove&&!(contentsMask&moveable))return false;
    if(excluded.handle){
        if(entity.handle==excluded.handle)return false;
        if(excluded.baseEntity&&
           (entity.owner==excluded.baseEntity||excluded.owner==entity.baseEntity))return false;
    }
    return entityRule(entity.baseEntity,particleGroup,contentsMask)&&
           groupRule(particleGroup,entity.collisionGroup);
}

// Default client entity policy for dynamic props and other entities verified
// to inherit the base virtual. Entity classes with overrides use their own rule.
inline bool sourceSfmParticleTraceShouldHitBaseEntityCandidate(
    const SourceParticleTraceEntityCandidate& entity,
    const SourceParticleTraceEntityCandidate& excluded,uint32_t contentsMask,int32_t particleGroup){
    return sourceParticleTraceShouldHitEntityCandidate(entity,excluded,contentsMask,particleGroup,
        [&](uintptr_t,int32_t,uint32_t mask){
            return sourceSfmBaseEntityCollisionCandidate(entity.collisionGroup,mask);
        },sourceSfmTfCollisionGroupsCandidate);
}
