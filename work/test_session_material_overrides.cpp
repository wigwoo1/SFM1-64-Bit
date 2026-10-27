#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_material_overrides_candidate.h"
#include "apply_session_material_overrides_candidate.h"
#include "mounted_model_material_candidate.h"
#include "session_material_binding_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include <fstream>
#include <iostream>
#include "material_test_fixtures.h"
static int integration(const char*session,const std::filesystem::path&content){try{if(!std::filesystem::is_regular_file(session))throw std::runtime_error("Integration fixture unavailable: private roundtrip session file missing");auto g=loadGraph(session);auto mounts=materialTestInstalledMounts(content);if(!readSessionMaterialOverridesCandidate(g,32170).empty())throw std::runtime_error("Awtter override binding");size_t base=0,resolved=0,missing=0;for(uint32_t model:{2552u,33602u,34561u}){auto records=readSessionMaterialOverridesCandidate(g,model);size_t count=0;for(const auto&r:records)count+=r.parameters.count("$basetexture");if(count!=(model==2552?4u:model==34561?3u:0u))throw std::runtime_error("base override coverage");for(const auto&record:records){auto found=record.parameters.find("$basetexture");if(found==record.parameters.end())continue;auto syntax=parseVmtCandidate("VertexLitGeneric { \"$basetexture\" \"old\" }");auto applied=applySessionMaterialOverridesCandidate(syntax,record);auto values=vmtValuesCandidate(applied,"$basetexture");if(values.size()!=1||values[0]!=found->second.values[0].text)throw std::runtime_error("base texture override application");auto texture=findMountedMaterialCandidate(mounts,materialRelativePathCandidate(values[0],".vtf"));if(texture.empty()){++missing;std::cout<<"override_texture_missing="<<values[0]<<"\n";}else{++resolved;ModelMaterialMetadataCandidate metadata;metadata.names={record.material};metadata.directories={""};metadata.skins={{0}};auto loaded=resolveMountedModelMaterialCandidate(metadata,0,0,mounts,sessionMaterialBindingCandidate(records,mounts));if(loaded.baseTexture!=texture)throw std::runtime_error("resolver ignored override");std::cout<<"override_texture="<<texture.string()<<"\n";}}auto key=sessionMaterialOverrideKeyCandidate(records);std::reverse(records.begin(),records.end());if(key!=sessionMaterialOverrideKeyCandidate(records))throw std::runtime_error("order dependent key");for(auto&r:records)if(!r.parameters.empty()){auto&parameter=r.parameters.begin()->second;if(parameter.type==5){parameter.values[0].text+="_changed";if(key==sessionMaterialOverrideKeyCandidate(records))throw std::runtime_error("override key stale");break;}}std::cout<<"model="<<model<<" material_overrides="<<records.size()<<" base_texture_overrides="<<count<<"\n";base+=count;}if(normalizeSessionMaterialNameCandidate("Models\\Example\\BODY.vmt")!="models/example/body")throw std::runtime_error("name normalization");auto&element=g.elements[2553];element.attributes.push_back(element.attributes.back());bool rejected=false;try{readSessionMaterialOverridesCandidate(g,2552);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("duplicate accepted");std::cout<<"override_texture_resolved="<<resolved<<" missing="<<missing<<"\n";std::cout<<"session material overrides passed base_parameters="<<base<<" material_identity_match=1 resolver_applies_before_lookup=1 override_merge=1 override_texture_lookup=1 gameinfo_language_mounts=1 scoped_binding=1 exact_cache_key_changes=1 order_independent=1 duplicate_rejected=1 renderer_application=unfinished\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}

int main(int argc,char**argv){try{
 materialTestGameInfo();
 if(argc==4&&std::string(argv[1])=="--integration")return integration(argv[2],argv[3]);
 if(argc!=3||std::string(argv[1])!="--fixtures")throw std::runtime_error("Use --fixtures FRESH_WORKSPACE_OUTPUT_DIR or --integration PRIVATE_ROUNDTRIP_SESSION CONTENT_DIRECTORY");
 auto mount=materialTestMount(argv[2]);auto g=materialTestGraph();auto records=readSessionMaterialOverridesCandidate(g,0);
 materialTestRequire(records.size()==1&&records[0].parameters.size()==2,"owned override graph coverage");
 auto original=sessionMaterialOverrideKeyCandidate(records);materialTestSetTexture(g,"fixture/alternate");records=readSessionMaterialOverridesCandidate(g,0);
 materialTestRequire(sessionMaterialOverrideKeyCandidate(records)!=original,"owned override cache key changes");
 ModelMaterialMetadataCandidate metadata;metadata.names={"models/fixture/visor"};metadata.directories={""};metadata.skins={{0}};
 auto resolved=resolveMountedModelMaterialCandidate(metadata,0,0,{mount},sessionMaterialBindingCandidate(records,{mount}));
 materialTestRequire(resolved.baseTexture==mount/"materials/fixture/alternate.vtf"&&!resolved.baseTextureMissing,"override applied before texture resolution");
 auto pixels=decodeVtfMipCandidate(readBinaryAssetCandidate(resolved.baseTexture),0);materialTestRequire(pixels.rgba==std::vector<uint8_t>{5,10,15,255},"owned override texture bytes");
 auto duplicate=g;duplicate.elements[0].attributes[0].values.push_back(duplicate.elements[0].attributes[0].values[0]);bool rejected=false;
 try{readSessionMaterialOverridesCandidate(duplicate,0);}catch(const std::runtime_error&){rejected=true;}materialTestRequire(rejected,"duplicate override still refused");
 std::cout<<"PASS owned_session_override_checks=5 integration=NOT_RUN\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
