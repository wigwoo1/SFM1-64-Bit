#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "scene_render_inputs_cache_candidate.h"
#include "session_material_overrides_candidate.h"
#include <iostream>
#include "material_test_fixtures.h"
static int exercise(const SfmModelGeometryCandidate&model,const std::filesystem::path&mount,const std::string&materialName,const std::string&maskName,bool owned){try{
 SceneRenderInputsCacheCandidate cache({mount},2,[&](const std::string&)->const SfmModelGeometryCandidate&{return model;});
 auto fetch=[&](const std::string&tint,const std::string&mask,bool enabled)->const ModelRenderInputsCandidate&{
  SessionMaterialOverrideCandidate override{0,materialName,{}};
  for(const auto&pair:std::vector<std::pair<std::string,std::string>>{{"$selfillum",enabled?"1":"0"},{"$selfillumtint",tint},{"$selfillummask",mask}}){Value value{};value.text=pair.second;override.parameters.emplace(pair.first,SessionMaterialParameterCandidate{5,{value}});}
  auto key=sessionMaterialOverrideKeyCandidate({override});
  auto apply=[&](const std::filesystem::path&path,VmtNodeCandidate&syntax){if(vmtAsciiLowerCandidate(path.stem().string())!="visor")return;syntax.children.erase(std::remove_if(syntax.children.begin(),syntax.children.end(),[&](const auto&node){return override.parameters.count(vmtAsciiLowerCandidate(node.key))!=0;}),syntax.children.end());for(const auto&parameter:override.parameters){VmtNodeCandidate node;node.key=parameter.first;node.value=parameter.second.values[0].text;syntax.children.push_back(std::move(node));}};
  return cache.get("mamagen",0,0,key,apply);
 };
 auto visor=[](const ModelRenderInputsCandidate&input)->const ModelRenderMaterialCandidate&{for(const auto&material:input.materials)if(vmtAsciiLowerCandidate(material.source.vmt.stem().string())=="visor")return material;throw std::runtime_error("visor absent");};
 auto&original=fetch("[5 5 5]",maskName,true);
 auto&changed=fetch("[.5 1 5]",maskName,true);
 if(&original==&changed||visor(changed).selfIllum.tint!=std::array<float,3>{.5f,1,5})throw std::runtime_error("changed tint reused stale cache");
 auto&disabled=fetch("[5 5 5]",maskName,false);
 if(&disabled==&original||visor(disabled).selfIllum.enabled||!visor(disabled).selfIllumMaskImage.rgba.empty())throw std::runtime_error("disabled mask cache state");
 if(&fetch("[5 5 5]",maskName,true)!=&original)throw std::runtime_error("restored cache identity lost");
 materialTestRequire(!visor(original).source.selfIllumMaskMissing&&!visor(original).selfIllumMaskImage.rgba.empty(),"authored mask decoded");
 if(owned)materialTestRequire(visor(original).selfIllumMaskImage.rgba==std::vector<uint8_t>{128,64,32,255},"owned mask exact pixel bytes");
 auto&missing=fetch("[5 5 5]","fixture/__absent_selfillum_mask__",true);materialTestFallback(visor(missing),true);
 if(cache.size()!=4||cache.hits()!=1||&fetch("[5 5 5]","fixture/__absent_selfillum_mask__",true)!=&missing||cache.hits()!=2)throw std::runtime_error("missing mask fallback cache identity/counts");
 bool rejected=false;try{fetch("[5 5 5]",owned?"fixture/broken":"../outside",true);}catch(const std::runtime_error&){rejected=true;}materialTestRequire(rejected&&cache.size()==4,"malformed or unsafe mask refusal not cached");
 std::cout<<"PASS selfillum cache: tint/flag distinct, restore reused, missing mask diagnostic/base-alpha fallback cached, malformed_or_unsafe_refused; entries=4 hits=2\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

int main(int argc,char**argv){try{
 if(argc==3&&std::string(argv[1])=="--fixtures"){auto mount=materialTestMount(argv[2]);return exercise(materialTestModel(),mount,"models/fixture/visor","fixture/glow",true);}
 if(argc==3&&std::string(argv[1])=="--integration"){
  std::filesystem::path mount=argv[2];auto base=mount/"models/anno/mamagen/mamagen";
  for(const auto&ext:{".mdl",".vvd",".dx90.vtx"})if(!std::filesystem::is_regular_file(base.string()+ext))throw std::runtime_error("Integration fixture unavailable: Mamagen model file missing: "+base.string()+ext);
  auto model=readSfmModelGeometryCandidate(readBinaryAssetCandidate(base.string()+".mdl"),readBinaryAssetCandidate(base.string()+".vvd"),readBinaryAssetCandidate(base.string()+".dx90.vtx"));return exercise(model,mount,"models/anno/mamagen/visor","models/anno/mamagen/visorglow",false);
 }
 throw std::runtime_error("Use --fixtures FRESH_WORKSPACE_OUTPUT_DIR or --integration CONTENT_DIRECTORY");
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
