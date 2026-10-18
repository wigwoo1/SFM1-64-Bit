#include "source_displacement_ray_tree_candidate.h"
#include "source_displacement_grid_triangles_candidate.h"
#include <iostream>
int main(){try{
 std::vector<std::array<float,3>>vertices;for(uint32_t y=0;y<3;++y)for(uint32_t x=0;x<3;++x)vertices.push_back({float(x*8),float(y*8),0});
 std::vector<std::array<uint16_t,3>>indices;for(const auto&t:sourceDisplacementGridTrianglesCandidate(1,std::vector<uint16_t>(8)))indices.push_back(t.vertices);
 std::vector<SourceDisplacementCollisionTriangleCandidate>triangles;for(const auto&i:indices){SourceDisplacementCollisionTriangleCandidate t;t.packedVertices=i;t.flags=0x7c0;triangles.push_back(sourceDisplacementCollisionTriangleCandidate(vertices,t));}
 const auto tree=sourceDisplacementCollisionTreeCandidate(1,vertices,indices);
 const std::array<float,3>start{.25f,.25f,1},delta{0,0,-2},inverse{std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),-.5f};
 auto run=[&](SourceDisplacementRayTreeHitCandidate prior){return sourceDisplacementRayTreeCandidate(vertices,triangles,tree,start,delta,inverse,true,prior);};
 auto hit=run({});if(hit.fraction!=.5f||hit.triangle!=0)throw std::runtime_error("analytical tree hit/first triangle tie");
 auto tied=run({.5f,UINT32_MAX});if(tied.fraction!=.5f||tied.triangle!=UINT32_MAX)throw std::runtime_error("equal prior hit changed");
 auto closer=run({.25f,7});if(closer.fraction!=.25f||closer.triangle!=7)throw std::runtime_error("closer prior hit changed");
 auto reordered=tree;std::swap(reordered.leafPairs[0][0],reordered.leafPairs[0][1]);auto order=sourceDisplacementRayTreeCandidate(vertices,triangles,reordered,start,delta,inverse,true);if(order.triangle!=1)throw std::runtime_error("leaf pair order not preserved");
 SourceDisplacementRayTraceCandidate trace;trace.fraction=.75f;if(!sourceDisplacementRayTraceCandidate(vertices,triangles,tree,0,1,start,delta,inverse,true,trace)||trace.fraction!=.5f||trace.normal!=std::array<float,3>{0,0,1}||trace.distance!=0||trace.displacementFlags!=31)throw std::runtime_error("analytical wrapper plane/tags");
 auto priorTrace=trace;if(sourceDisplacementRayTraceCandidate(vertices,triangles,tree,8,1,start,delta,inverse,true,trace)||trace.fraction!=priorTrace.fraction||trace.displacementFlags!=31)throw std::runtime_error("disabled wrapper changed trace");
 if(sourceDisplacementRayTraceCandidate(vertices,triangles,tree,0,0,start,delta,inverse,true,trace))throw std::runtime_error("contents gate");
 unsigned rejected=0;auto rejects=[&](const std::function<void()>&f){try{f();}catch(const std::runtime_error&){++rejected;return;}throw std::runtime_error("invalid input accepted");};
 auto badTree=tree;badTree.nodes.clear();rejects([&]{sourceDisplacementRayTreeCandidate(vertices,triangles,badTree,start,delta,inverse,true);});
 badTree=tree;badTree.leafPairs.pop_back();rejects([&]{sourceDisplacementRayTreeCandidate(vertices,triangles,badTree,start,delta,inverse,true);});
 badTree=tree;badTree.leafPairs[0][0]=65535;rejects([&]{sourceDisplacementRayTreeCandidate(vertices,triangles,badTree,start,delta,inverse,true);});
 badTree=tree;badTree.nodes[0].bounds[0]=std::numeric_limits<float>::infinity();rejects([&]{sourceDisplacementRayTreeCandidate(vertices,triangles,badTree,start,delta,inverse,true);});
 auto badTriangles=triangles;badTriangles[0].packedVertices[0]=511;rejects([&]{sourceDisplacementRayTreeCandidate(vertices,badTriangles,tree,start,delta,inverse,true);});
 auto badInverse=inverse;badInverse[0]=std::numeric_limits<float>::infinity();rejects([&]{sourceDisplacementRayTreeCandidate(vertices,triangles,tree,start,delta,badInverse,true);});
 rejects([&]{run({2.f,UINT32_MAX});});rejects([&]{run({1.f,999});});
 // Failure before commit must retain the entire wrapper state.
 trace=SourceDisplacementRayTraceCandidate{};const auto before=trace;bool failed=false;try{sourceDisplacementRayTraceCandidate(vertices,badTriangles,tree,0,1,start,delta,inverse,true,trace);}catch(const std::runtime_error&){failed=true;}if(!failed||trace.fraction!=before.fraction||trace.normal!=before.normal||trace.distance!=before.distance||trace.displacementFlags!=before.displacementFlags)throw std::runtime_error("failed wrapper changed state or accepted invalid data");
 if(rejected!=8)throw std::runtime_error("guard coverage");std::cout<<"PASS hit/tie/prior/order/wrapper/filter cases, eight guards and transactional failure\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
