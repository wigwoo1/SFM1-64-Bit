#include "source_prepared_world_ray_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("map required");std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("map read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});
 SourceBspBrushOnlyTraceCandidate world(b,std::vector<uint32_t>(32768));std::vector<SourceDisplacementRuntimeMeshCandidate>meshes;SourcePreparedWorldRayCandidate prepared(world,meshes);
 SourcePreparedWorldRayStateCandidate state;state.trace.plane={99,98,97,96};state.displacementFlags=0xbeef;state.displacementHit=1;auto before=state;
 auto same=[](const auto&a,const auto&b){return a.trace.fraction==b.trace.fraction&&a.trace.leftSolid==b.trace.leftSolid&&a.trace.startSolid==b.trace.startSolid&&a.trace.allSolid==b.trace.allSolid&&a.trace.contents==b.trace.contents&&a.trace.plane==b.trace.plane&&a.displacementFlags==b.displacementFlags&&a.displacementHit==b.displacementHit;};
 unsigned rejected=0;auto rejects=[&](const std::function<void()>&run){try{run();}catch(const std::runtime_error&){++rejected;if(!same(state,before))throw std::runtime_error("failure changed world state");return;}throw std::runtime_error("invalid input accepted");};
 rejects([&]{prepared.trace({0,0,0},{1,1,1},0x2000,state);});
 rejects([&]{prepared.trace({0,0,0},{std::numeric_limits<float>::infinity(),1,1},1,state);});
 const auto oldRoot=world.root;world.root=int32_t(world.nodes.size()+1);rejects([&]{prepared.trace({0,0,0},{1,1,1},1,state);});world.root=oldRoot;
 const std::array<float,3>mins{-1,-1,-1},maxs{1,1,1};SourceOptimizedBoxLineStateCandidate box;box.plane={-1,0,0,1};bool entering=false;
 // Same plane as the prior result: comparing plane bytes cannot detect entry.
 sourceOptimizedBoxLineRayCandidate(mins,maxs,{-2,0,0},{2,0,0},1,box,&entering);if(!entering||box.plane!=std::array<float,4>{-1,0,0,1})throw std::runtime_error("entering event lost on identical plane");
 entering=false;box={};sourceOptimizedBoxLineRayCandidate(mins,maxs,{0,0,0},{0,0,0},1,box,&entering);if(entering||!box.allSolid)throw std::runtime_error("solid branch incorrectly signalled entry");
 entering=false;box={};sourceOptimizedBoxLineRayCandidate(mins,maxs,{0,0,0},{2,0,0},1,box,&entering);if(entering||!box.startSolid||box.allSolid)throw std::runtime_error("exit branch incorrectly signalled entry");
 entering=false;box={};sourceOptimizedBoxLineRayCandidate(mins,maxs,{2,2,2},{3,3,3},1,box,&entering);if(entering||box.fraction!=1)throw std::runtime_error("miss branch incorrectly signalled entry");
 if(rejected!=3)throw std::runtime_error("rejection coverage");std::cout<<"PASS identical-plane entry event, solid/exit/miss no-event, and three transactional world rejections\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
