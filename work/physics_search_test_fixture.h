#pragma once
#include "source_physics_search_candidate.h"
#include "physics_support_test_fixture.h"
struct PhysicsSearchInputFixture {
    std::array<float,16>ray{};
    SourcePhysicsBoxSupportCandidate box;
    PhysicsSupportInputFixture shape;
    SourcePhysicsSearchStateCandidate state;
    float tolerance=0;
};
inline void searchWriteTrace(std::ostream&out,const SourcePhysicsSearchTraceCandidate&t){supportPut(out,t.start);supportPut(out,t.end);supportPut(out,t.plane.normal);supportPut(out,t.plane.distance);supportPut(out,t.planeTags);supportPut(out,t.fraction);supportPut(out,t.contents);supportPut(out,t.displacementFlags);supportPut(out,t.allSolid);supportPut(out,t.startSolid);supportPut(out,t.fractionLeftSolid);supportPut(out,t.surfaceId);supportPut(out,t.surfaceMetadata);supportPut(out,t.hitGroup);supportPut(out,t.physicsBone);supportPut(out,t.worldSurface);supportPut(out,t.entityId);supportPut(out,t.hitBox);}
inline void searchReadTrace(std::istream&in,SourcePhysicsSearchTraceCandidate&t){supportGet(in,t.start);supportGet(in,t.end);supportGet(in,t.plane.normal);supportGet(in,t.plane.distance);supportGet(in,t.planeTags);supportGet(in,t.fraction);supportGet(in,t.contents);supportGet(in,t.displacementFlags);supportGet(in,t.allSolid);supportGet(in,t.startSolid);supportGet(in,t.fractionLeftSolid);supportGet(in,t.surfaceId);supportGet(in,t.surfaceMetadata);supportGet(in,t.hitGroup);supportGet(in,t.physicsBone);supportGet(in,t.worldSurface);supportGet(in,t.entityId);supportGet(in,t.hitBox);}
inline void searchWriteState(std::ostream&out,const SourcePhysicsSearchStateCandidate&s){searchWriteTrace(out,s.trace);supportPut(out,s.direction);supportPut(out,s.distance);}
inline void searchReadState(std::istream&in,SourcePhysicsSearchStateCandidate&s){searchReadTrace(in,s.trace);supportGet(in,s.direction);supportGet(in,s.distance);}
inline void searchWriteInput(std::ostream&out,const PhysicsSearchInputFixture&f){supportPut(out,f.ray);supportPut(out,f.box.axes);supportPut(out,uint32_t(f.box.point));supportWriteInput(out,f.shape);searchWriteState(out,f.state);supportPut(out,f.tolerance);}
inline bool searchReadInput(std::istream&in,PhysicsSearchInputFixture&f){if(in.peek()==std::char_traits<char>::eof())return false;supportGet(in,f.ray);supportGet(in,f.box.axes);uint32_t point;supportGet(in,point);if(point>1)throw std::runtime_error("search box flag");f.box.point=point!=0;if(!supportReadInput(in,f.shape))throw std::runtime_error("missing search shape");searchReadState(in,f.state);supportGet(in,f.tolerance);return true;}
inline void searchWriteOutput(std::ostream&out,const SourcePhysicsSearchStateCandidate&s,bool answer,const SourcePhysicsSupportCacheCandidate&cache,uint32_t mode){supportPut(out,uint32_t(112+(mode==0?2050:0)));searchWriteState(out,s);supportPut(out,uint32_t(answer));if(mode==0){supportPut(out,cache.entries);supportPut(out,cache.generation);}}
