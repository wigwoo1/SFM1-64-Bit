#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_session_scene_candidate.h"
#include "retained_static_flex_provider_candidate.h"
#include "gameinfo_mounts_candidate.h"
int main(){try{
 std::filesystem::path content=R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod)";std::ifstream file(content/"gameinfo.txt",std::ios::binary);if(!file)throw std::runtime_error("GameInfo missing");std::string text((std::istreambuf_iterator<char>(file)),{});auto mounts=gameinfoMountsCandidate(text,content,content.parent_path());
 RetainedSessionSceneCandidate scene(loadGraph("work/roundtrip-11.dmx"),2237,mounts,4,true,true);RetainedStaticFlexProviderCandidate provider(mounts);std::map<uint32_t,VvdGeometryCandidate>buffers;std::map<uint32_t,std::pair<const void*,const void*>>storage;
 std::ofstream csv("outputs/flex-buffer-reuse-profile.csv");if(!csv)throw std::runtime_error("profile output unavailable");csv<<"frame,model,reuse,copy_ms,processing_ms,deformation_ms\n";csv.precision(9);int frameNumber=0;
 scene.setBorrowedDeformationCandidate([&](const Graph&graph,const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&model)->const VvdGeometryCandidate&{
  auto&reused=buffers[instance.id];VvdGeometryCandidate fresh;StaticFlexStageProfileCandidate freshProfile,reuseProfile;
  auto runFresh=[&]{provider.sampleInto(graph,instance,model,fresh,&freshProfile);};auto runReuse=[&]{provider.sampleInto(graph,instance,model,reused,&reuseProfile);};
  if(frameNumber%2){runReuse();runFresh();}else{runFresh();runReuse();}
  if(fresh.checksum!=reused.checksum||fresh.lod!=reused.lod||fresh.vertices!=reused.vertices||fresh.tangents!=reused.tangents)throw std::runtime_error("reuse changed flex output");
  std::pair<const void*,const void*>addresses{reused.vertices.data(),reused.tangents.data()};auto inserted=storage.emplace(instance.id,addresses);if(!inserted.second&&inserted.first->second!=addresses)throw std::runtime_error("reused storage reallocated");
  for(int reuse=0;reuse<2;++reuse){const auto&p=reuse?reuseProfile:freshProfile;csv<<frameNumber<<','<<instance.id<<','<<reuse<<','<<p.copy<<','<<p.processing<<','<<p.deformation<<'\n';}return reused;
 });
 for(frameNumber=0;frameNumber<180;++frameNumber){auto frame=scene.sample(1259500+(frameNumber%61)*10000/60);if(frame.upload.vertices.size()!=110396)throw std::runtime_error("profile workload changed");}
 if(buffers.size()!=4||provider.metadataCount()!=4||!csv)throw std::runtime_error("profile invariant failed");std::cout<<"paired flex profile passed 180 frames four models exact_output stable_storage alternating_order\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
