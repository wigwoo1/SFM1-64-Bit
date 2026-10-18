#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "source_bsp_point_leaf_candidate.h"
#include "source_bsp_leaf_ambient_candidate.h"
#include <iomanip>
#include <iostream>
#include <limits>
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=4)throw std::runtime_error("session.dmx gameinfo.txt map.bsp required");
 auto graph=loadGraph(argv[1]);auto root=sessionRenderRootCandidate(graph);const auto bytes=readBinaryAssetCandidate(argv[3]);SourceBspPointLeafCandidate tree(bytes);SourceBspLeafAmbientCandidate ambient(bytes,SourceBspAmbientEncodingCandidate::Hdr);
 if(tree.leafCount()!=ambient.leaves.size())throw std::runtime_error("spatial/ambient leaf coverage");
 std::cout<<std::setprecision(9)<<"nodes="<<tree.nodeCount()<<" planes="<<tree.planeCount()<<" leaves="<<tree.leafCount()<<" world_root="<<tree.worldRoot()<<'\n';
 auto query=[&](const char*kind,size_t model,const std::array<float,3>&point){auto leaf=tree.leafAt(point);auto owner=ambient.leaves[leaf].sampleOwner;
  std::cout<<"query kind="<<kind<<" model="<<model<<" point="<<point[0]<<','<<point[1]<<','<<point[2]<<" leaf="<<leaf<<" ambient_owner="<<(owner?int64_t(*owner):-1)<<" samples="<<(owner?ambient.leaves[*owner].count:0)<<'\n';};
 // Both are diagnostic query choices, not assertions about native lighting anchors.
 RetainedShotTimelineCandidate timeline(graph,root);auto step=evaluateLiveSessionSceneCandidate(timeline,50000,1280,720);
 for(size_t model=0;model<step.scene.models.size();++model){const auto&rows=step.scene.models[model].placement.matrix.rows;query("transform_origin",model,{rows[3],rows[7],rows[11]});}
 auto textBytes=readBinaryAssetCandidate(argv[2]);std::string text(textBytes.begin(),textBytes.end());auto content=std::filesystem::path(argv[2]).parent_path();auto mounts=gameinfoMountsCandidate(text,content,content.parent_path());RetainedSessionSceneCandidate scene(std::move(graph),root,mounts,4,true,true);const auto frame=scene.sample(50000,1280,720);
 for(size_t model=0;model<frame.materialInputs.size();++model){std::array<float,3>minimum{INFINITY,INFINITY,INFINITY},maximum{-INFINITY,-INFINITY,-INFINITY};bool found=false;
  auto bounds=[&](const auto&draws){for(const auto&draw:draws)if(draw.model==model)for(size_t i=draw.firstIndex;i<size_t(draw.firstIndex)+draw.indexCount;++i){const auto&point=frame.upload.vertices.at(frame.upload.indices.at(i)).position;for(size_t axis=0;axis<3;++axis){minimum[axis]=std::min(minimum[axis],point[axis]);maximum[axis]=std::max(maximum[axis],point[axis]);}found=true;}};
  bounds(frame.upload.opaque);bounds(frame.upload.translucent);if(found)query("rendered_bounds_center",model,{(minimum[0]+maximum[0])*.5f,(minimum[1]+maximum[1])*.5f,(minimum[2]+maximum[2])*.5f});
 }
 std::cout<<"PASS: diagnostic points map to ambient tables; native lighting origin/interpolation UNVERIFIED\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
