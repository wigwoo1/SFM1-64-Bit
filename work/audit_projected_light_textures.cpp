#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "projected_light_saved_fields_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "mounted_model_material_candidate.h"
#include "vtf_block_decode.h"
#include <map>
#include "projected_light_texture_frame_candidate.h"
int main(){try{
 const std::filesystem::path game=R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game)";
 std::ifstream configuration(game/"usermod"/"gameinfo.txt",std::ios::binary);if(!configuration)throw std::runtime_error("gameinfo read");
 const std::string source((std::istreambuf_iterator<char>(configuration)),{});
 const auto mounts=gameinfoMountsCandidate(source,game/"usermod",game);
 std::map<std::string,unsigned> names; std::map<std::string,std::vector<ProjectedLightSavedFieldsCandidate>> savedLights;
 for(const char* path:{"work/roundtrip-11.dmx","work/roundtrip-139.dmx"}){const auto graph=loadGraph(path);
  for(size_t index=0;index<graph.elements.size();++index)if(graph.strings.at(graph.elements[index].type)=="DmeProjectedLight"){auto saved=projectedLightSavedFieldsCandidate(graph,index);++names[saved.textureName];savedLights[saved.textureName].push_back(saved);}
 }
 for(const auto& entry:names){std::cout<<"name="<<entry.first<<" lights="<<entry.second;
  const auto path=findMountedMaterialCandidate(mounts,materialRelativePathCandidate(entry.first,".vtf"));
  if(path.empty()){std::cout<<" loose_asset=missing\n";continue;}
  std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("light VTF read");const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
  std::cout<<" path="<<path.string()<<" bytes="<<bytes.size();
  if(bytes.size()>=26){uint16_t frames;uint32_t minor;std::memcpy(&frames,bytes.data()+24,2);std::memcpy(&minor,bytes.data()+8,4);std::cout<<" header_frames="<<frames<<" header_minor="<<minor;}
  try{const auto layout=readVtfTextureLayoutCandidate(bytes);const auto image=decodeVtfMipCandidate(bytes,0);std::cout<<" decoded="<<image.width<<'x'<<image.height<<" format="<<layout.format<<" rgba_bytes="<<image.rgba.size(); unsigned frameChecks=0;for(const auto& saved:savedLights.at(entry.first)){const auto selected=projectedLightTextureFrameCandidate(saved.animationTimeWord,saved.floatWords[10],true,int32_t(layout.frameCount));if(selected!=0)throw std::runtime_error("single frame selection mismatch");++frameChecks;}std::cout<<" validated_frames="<<layout.frameCount<<" light_frame_checks="<<frameChecks;}
  catch(const std::runtime_error& e){std::cout<<" decoder_limit="<<e.what();}
  std::cout<<'\n';
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
