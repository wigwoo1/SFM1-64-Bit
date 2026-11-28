#pragma once
#include "source_physics_search_candidate.h"
#include "source_world_light_entity_filter_candidate.h"

// Semantic view of the valid world record borrowed by server1016d840.
// Flag bit0x04 permits its entity field; other bits do not affect this read.
struct SourceServerTraceWorldBindingCandidate {
 uint8_t flags=0;
 uintptr_t entity=0;
};
// Full server annotation, distinct from client1016d7b0. A supplied valid
// collideable's virtual0 resolves a nonnull handle object. Production record
// ownership and handle lifetime remain the scene owner's responsibility.
template<class Services>
void sourceServerTraceAnnotationCandidate(uintptr_t collideable,
 SourcePhysicsSearchTraceCandidate& trace,const SourceServerTraceWorldBindingCandidate& world,
 Services& services){
 if(!(trace.fraction<1.f||trace.allSolid||trace.startSolid))return;
 const auto entity=services.collideableEntity(collideable);
 if(!sourceStaticPropEntityCandidate(entity,services)){trace.entityId=entity;return;}
 trace.entityId=(world.flags&4)?world.entity:0;
 trace.hitBox=sourceStaticPropEntityIndexCandidate(entity,services)+1;
}
