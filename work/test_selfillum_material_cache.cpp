#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "scene_render_inputs_cache_candidate.h"
#include "session_material_overrides_candidate.h"
#include <iostream>
int main(){try{
 std::filesystem::path mount=R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod)";
 auto base=mount/"models/anno/mamagen/mamagen";
 auto model=readSfmModelGeometryCandidate(readBinaryAssetCandidate(base.string()+".mdl"),readBinaryAssetCandidate(base.string()+".vvd"),readBinaryAssetCandidate(base.string()+".dx90.vtx"));
 SceneRenderInputsCacheCandidate cache({mount},2,[&](const std::string&)->const SfmModelGeometryCandidate&{return model;});
 auto fetch=[&](const std::string&tint,const std::string&mask,bool enabled)->const ModelRenderInputsCandidate&{
  SessionMaterialOverrideCandidate override{0,"models/anno/mamagen/visor",{}};
  for(const auto&pair:std::vector<std::pair<std::string,std::string>>{{"$selfillum",enabled?"1":"0"},{"$selfillumtint",tint},{"$selfillummask",mask}}){Value value{};value.text=pair.second;override.parameters.emplace(pair.first,SessionMaterialParameterCandidate{5,{value}});}
  auto key=sessionMaterialOverrideKeyCandidate({override});
  auto apply=[&](const std::filesystem::path&path,VmtNodeCandidate&syntax){if(vmtAsciiLowerCandidate(path.stem().string())!="visor")return;syntax.children.erase(std::remove_if(syntax.children.begin(),syntax.children.end(),[&](const auto&node){return override.parameters.count(vmtAsciiLowerCandidate(node.key))!=0;}),syntax.children.end());for(const auto&parameter:override.parameters){VmtNodeCandidate node;node.key=parameter.first;node.value=parameter.second.values[0].text;syntax.children.push_back(std::move(node));}};
  return cache.get("mamagen",0,0,key,apply);
 };
 auto visor=[](const ModelRenderInputsCandidate&input)->const ModelRenderMaterialCandidate&{for(const auto&material:input.materials)if(vmtAsciiLowerCandidate(material.source.vmt.stem().string())=="visor")return material;throw std::runtime_error("visor absent");};
 auto&original=fetch("[5 5 5]","models/anno/mamagen/visorglow",true);
 auto&changed=fetch("[.5 1 5]","models/anno/mamagen/visorglow",true);
 if(&original==&changed||visor(changed).selfIllum.tint!=std::array<float,3>{.5f,1,5})throw std::runtime_error("changed tint reused stale cache");
 auto&disabled=fetch("[5 5 5]","models/anno/mamagen/visorglow",false);
 if(&disabled==&original||visor(disabled).selfIllum.enabled||!visor(disabled).selfIllumMaskImage.rgba.empty())throw std::runtime_error("disabled mask cache state");
 if(&fetch("[5 5 5]","models/anno/mamagen/visorglow",true)!=&original)throw std::runtime_error("restored cache identity lost");
 auto entries=cache.size();bool rejected=false;
 try{fetch("[5 5 5]","models/anno/mamagen/__missing_selfillum_cache_mask__",true);}catch(const std::runtime_error&){rejected=true;}
 if(!rejected||cache.size()!=entries||entries!=3||cache.hits()!=1)throw std::runtime_error("failed mask cached or cache counts wrong");
 std::cout<<"PASS installed model selfillum cache: tint/flag changes distinct, restore identity reused, missing mask rejected without caching; entries=3 hits=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
