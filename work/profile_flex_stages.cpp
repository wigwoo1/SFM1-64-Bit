#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_session_scene_candidate.h"
#include "retained_static_flex_provider_candidate.h"
#include "gameinfo_mounts_candidate.h"
int main(){try{
 std::filesystem::path content=R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod)";std::ifstream file(content/"gameinfo.txt",std::ios::binary);if(!file)throw std::runtime_error("GameInfo missing");std::string text((std::istreambuf_iterator<char>(file)),{});auto mounts=gameinfoMountsCandidate(text,content,content.parent_path());
 RetainedSessionSceneCandidate scene(loadGraph("work/roundtrip-11.dmx"),2237,mounts,4,true,true);auto provider=std::make_shared<RetainedStaticFlexProviderCandidate>(mounts);std::vector<std::pair<uint32_t,StaticFlexStageProfileCandidate>>profiles;
 scene.setModelDeformationCandidate([&](const Graph&graph,const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&model){StaticFlexStageProfileCandidate p;auto geometry=provider->sample(graph,instance,model,&p);profiles.emplace_back(instance.id,p);return geometry;});
 std::ofstream csv("outputs/flex-stage-profile.csv");if(!csv)throw std::runtime_error("profile output unavailable");csv<<"frame,model,metadata_ms,controllers_ms,rules_ms,deformation_ms,copy_ms,processing_ms\n";csv.precision(9);
 for(int n=0;n<240;++n){profiles.clear();auto frame=scene.sample(1259500+(n%61)*10000/60);if(profiles.size()!=4||frame.upload.vertices.size()!=110396)throw std::runtime_error("profile workload changed");for(const auto&entry:profiles){const auto&p=entry.second;csv<<n<<','<<entry.first<<','<<p.metadata<<','<<p.controllers<<','<<p.rules<<','<<p.deformation<<','<<p.copy<<','<<p.processing<<'\n';}}
 if(provider->metadataCount()!=4||!csv)throw std::runtime_error("profile cache/output invariant");std::cout<<"flex stage profile passed 240 four-model frames metadata_entries=4\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
