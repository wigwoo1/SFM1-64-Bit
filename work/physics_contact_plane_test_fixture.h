#pragma once
#include "source_physics_contact_plane_candidate.h"
#include "physics_refinement_test_fixture.h"
struct PhysicsContactPlaneInputFixture {PhysicsRefinementInputFixture data;std::array<uint8_t,84>trace{};};
inline void contactPlaneWriteInput(std::ostream&out,const PhysicsContactPlaneInputFixture&f){refinementWriteInput(out,f.data);supportPut(out,f.trace);}
inline bool contactPlaneReadInput(std::istream&in,PhysicsContactPlaneInputFixture&f){if(!refinementReadInput(in,f.data))return false;supportGet(in,f.trace);return true;}
inline void contactPlaneInvokeOwned(const PhysicsContactPlaneInputFixture&f,std::array<uint8_t,84>&output){std::array<float,3>end;std::memcpy(end.data(),output.data()+12,12);const auto result=sourcePhysicsContactPlaneCandidate(f.data.simplex,f.data.ray,f.data.box,f.data.shape.context,end);std::memcpy(output.data()+24,result.normal.data(),12);std::memcpy(output.data()+36,&result.distance,4);}
