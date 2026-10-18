#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include <iostream>
#include <map>

int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 audit required");
 if(argc!=3)throw std::runtime_error("session.dmx gameinfo.txt required");
 auto graph=loadGraph(argv[1]);auto root=sessionRenderRootCandidate(graph);
 auto bytes=readBinaryAssetCandidate(argv[2]);std::string text(bytes.begin(),bytes.end());
 auto content=std::filesystem::path(argv[2]).parent_path();
 auto mounts=gameinfoMountsCandidate(text,content,content.parent_path());
 RetainedSessionSceneCandidate owner(std::move(graph),root,mounts,4,true,true);
 auto frame=owner.sample(50000,1280,720);
 std::cout<<"frame=120 tick=50000 shot="<<frame.shot<<" projected_lights="<<frame.lightSnapshots.size()
          <<" material_groups="<<frame.materialInputs.size()<<" material_slots="<<frame.upload.materials.size()<<'\n';
 std::map<std::string,size_t> shaders,features;
 for(size_t group=0;group<frame.materialInputs.size();++group){
  const auto&inputs=frame.materialInputs.at(group);
  for(size_t index=0;index<inputs.materials.size();++index){
   const auto&m=inputs.materials[index];const auto&syntax=m.source.syntax;
   ++shaders[vmtAsciiLowerCandidate(syntax.key)];
   std::cout<<"material group="<<group<<" index="<<index<<" shader="<<syntax.key
     <<" path="<<m.source.vmt.string()<<" decoded="<<m.image.width<<'x'<<m.image.height
     <<" translucent="<<m.translucent<<" vertex_lightmap="<<m.vertexLighting
     <<" alpha_cutoff="<<m.alphaCutoff<<" selfillum="<<m.selfIllum.enabled<<'\n';
   for(const auto&field:syntax.children){
    ++features[vmtAsciiLowerCandidate(field.key)];
    if(group<3)std::cout<<"  "<<field.key<<'='<<(field.block?"<block>":field.value)<<'\n';
   }
  }
 }
 for(const auto&entry:shaders)std::cout<<"shader_count "<<entry.first<<'='<<entry.second<<'\n';
 for(const auto&entry:features)std::cout<<"authored_feature_count "<<entry.first<<'='<<entry.second<<'\n';
 std::cout<<"scope=evaluated_inputs_only shader_effects_and_native_parity=unverified\n";
 return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
