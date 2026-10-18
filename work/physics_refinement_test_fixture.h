#pragma once
#include "source_physics_refinement_candidate.h"
#include "physics_support_test_fixture.h"
struct PhysicsRefinementInputFixture {
    SourcePhysicsSimplexCandidate simplex;
    std::array<float,16>ray{};
    SourcePhysicsBoxSupportCandidate box;
    PhysicsSupportInputFixture shape;
};
inline void refinementWriteInput(std::ostream&out,const PhysicsRefinementInputFixture&f){supportPut(out,f.simplex);supportPut(out,f.ray);supportPut(out,f.box.axes);supportPut(out,uint32_t(f.box.point));supportWriteInput(out,f.shape);}
inline bool refinementReadInput(std::istream&in,PhysicsRefinementInputFixture&f){
    if(in.peek()==std::char_traits<char>::eof())return false;supportGet(in,f.simplex);supportGet(in,f.ray);supportGet(in,f.box.axes);uint32_t point;supportGet(in,point);if(point>1)throw std::runtime_error("refinement box flag");f.box.point=point!=0;if(!supportReadInput(in,f.shape))throw std::runtime_error("missing refinement support fixture");return true;
}
inline void refinementWriteOutput(std::ostream&out,const SourcePhysicsSimplexCandidate&s,float result,const SourcePhysicsSupportCacheCandidate&cache,uint32_t mode){supportPut(out,uint32_t(72+(mode==0?2050:0)));supportPut(out,s);supportPut(out,result);if(mode==0){supportPut(out,cache.entries);supportPut(out,cache.generation);}}
