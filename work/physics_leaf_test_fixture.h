#pragma once
#include "source_physics_leaf_candidate.h"
#include "physics_search_test_fixture.h"
#include "physics_tree_traversal_test_fixture.h"
struct PhysicsLeafInputFixture {
    SourcePhysicsLeafStateCandidate state;
    SourcePhysicsBoxSupportCandidate box;
    uint32_t contents=1,mask=0;
    bool prepared=false;
    PhysicsSupportInputFixture shape;
};
inline void leafWriteState(std::ostream&out,const SourcePhysicsLeafStateCandidate&s){searchWriteState(out,s.search);searchWriteTrace(out,s.trace);supportPut(out,s.ray);treeWriteState(out,s.tree);supportPut(out,s.localDelta);supportPut(out,s.nearest);supportPut(out,s.tolerance);}
inline void leafReadState(std::istream&in,SourcePhysicsLeafStateCandidate&s){searchReadState(in,s.search);searchReadTrace(in,s.trace);supportGet(in,s.ray);s.tree=treeReadState(in);supportGet(in,s.localDelta);supportGet(in,s.nearest);supportGet(in,s.tolerance);}
inline void leafWriteInput(std::ostream&out,const PhysicsLeafInputFixture&f){leafWriteState(out,f.state);supportPut(out,f.box.axes);supportPut(out,uint32_t(f.box.point));supportPut(out,f.contents);supportPut(out,f.mask);supportPut(out,uint32_t(f.prepared));if(f.prepared)supportWriteInput(out,f.shape);}
inline bool leafReadInput(std::istream&in,PhysicsLeafInputFixture&f){if(in.peek()==std::char_traits<char>::eof())return false;f={};leafReadState(in,f.state);supportGet(in,f.box.axes);uint32_t point,prepared;supportGet(in,point);supportGet(in,f.contents);supportGet(in,f.mask);supportGet(in,prepared);if(point>1||prepared>1||bool(prepared)!=bool(f.contents&f.mask))throw std::runtime_error("leaf fixture flags");f.box.point=point!=0;f.prepared=prepared!=0;if(f.prepared&&!supportReadInput(in,f.shape))throw std::runtime_error("leaf support missing");return true;}
inline void leafWriteOutput(std::ostream&out,const SourcePhysicsLeafStateCandidate&s,uint32_t code,const SourcePhysicsSupportCacheCandidate&cache,bool walk){supportPut(out,uint32_t(332+(walk?2050:0)));leafWriteState(out,s);supportPut(out,code);if(walk){supportPut(out,cache.entries);supportPut(out,cache.generation);}}
inline std::string leafStateBytes(const SourcePhysicsLeafStateCandidate&s){std::ostringstream out(std::ios::binary);leafWriteState(out,s);return out.str();}
