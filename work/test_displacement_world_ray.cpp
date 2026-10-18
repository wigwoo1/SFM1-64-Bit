#include "source_displacement_world_ray_candidate.h"
#include <cfloat>
#include <iostream>
int main(){try{
 SourceDisplacementRuntimeMeshCandidate mesh;mesh.contents=1;mesh.power=1;
 for(unsigned y=0;y<3;++y)for(unsigned x=0;x<3;++x)mesh.vertices.push_back({float(x*8),float(y*8),0});
 const auto raw=sourceDisplacementGridTrianglesCandidate(1,std::vector<uint16_t>(8));
 std::vector<std::array<uint16_t,3>>indices;
 for(const auto&t:raw){indices.push_back(t.vertices);SourceDisplacementCollisionTriangleCandidate tri;tri.packedVertices=t.vertices;tri.flags=0x40;mesh.triangles.push_back(sourceDisplacementCollisionTriangleCandidate(mesh.vertices,tri));}
 mesh.tree=sourceDisplacementCollisionTreeCandidate(1,mesh.vertices,indices);
 const std::array<float,3>start{.25f,.25f,1},delta{0,0,-2},inverse{FLT_MAX,FLT_MAX,-.5f};
 const std::array<uint16_t,2>properties{123,456};
 SourceDisplacementWorldRayStateCandidate initial;initial.contents=99;initial.surfaceProperty=77;initial.surfaceFlags=88;
 auto same=[](const auto&a,const auto&b){return a.ray.fraction==b.ray.fraction&&a.ray.normal==b.ray.normal&&a.ray.distance==b.ray.distance&&a.ray.displacementFlags==b.ray.displacementFlags&&a.contents==b.contents&&a.displacementHit==b.displacementHit&&a.surfaceIsDisplacement==b.surfaceIsDisplacement&&a.surfaceProperty==b.surfaceProperty&&a.surfaceFlags==b.surfaceFlags;};
 auto run=[&](auto&state){sourceDisplacementWorldRayCandidate(mesh,properties,start,delta,inverse,state);};
 auto state=initial;run(state);
 if(state.ray.fraction!=.5f||state.ray.normal!=std::array<float,3>{0,0,1}||state.ray.distance!=0||state.ray.displacementFlags!=1||state.contents!=1||state.displacementHit!=1||!state.surfaceIsDisplacement||state.surfaceProperty!=123||state.surfaceFlags!=0)throw std::runtime_error("first surface analytical hit");
 for(auto&t:mesh.triangles)t.flags|=0x400;state=initial;run(state);if(state.surfaceProperty!=456||state.ray.displacementFlags!=17)throw std::runtime_error("second surface flag selection");
 auto tied=state;run(state);if(!same(state,tied))throw std::runtime_error("equal fraction changed state");
 state=initial;state.ray.fraction=.25f;auto before=state;run(state);if(!same(state,before))throw std::runtime_error("closer prior changed state");
 state=initial;mesh.runtimeFlags=8;run(state);if(!same(state,initial))throw std::runtime_error("disabled mesh changed state");
 mesh.runtimeFlags=0;mesh.contents=0x1000;run(state);if(!same(state,initial))throw std::runtime_error("contents gate changed state");mesh.contents=1;
 sourceDisplacementWorldRayCandidate(mesh,properties,{.25f,.25f,-1},{0,0,2},{FLT_MAX,FLT_MAX,.5f},state);if(!same(state,initial))throw std::runtime_error("one-sided back face changed state");
 sourceDisplacementWorldRayCandidate(mesh,properties,{100,100,1},delta,inverse,state);if(!same(state,initial))throw std::runtime_error("miss changed state");
 mesh.tree.leafPairs[0][0]=65535;bool rejected=false;try{run(state);}catch(const std::runtime_error&){rejected=true;}if(!rejected||!same(state,initial))throw std::runtime_error("invalid mesh did not preserve state");
 std::cout<<"PASS both surface selections, one-sided hit, tie/prior/filter/miss retention and transactional failure\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
