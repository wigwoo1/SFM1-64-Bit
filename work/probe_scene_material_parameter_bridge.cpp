// Read-only integration diagnostic. Uses actual retained scene/material inputs;
// controlled CPU profiles are NOT discovered settings of the original SFM.
#define NOMINMAX
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "session_model_material_proxy_context_candidate.h"
#include "material_object_store_candidate.h"
#include "material_weapon_invis_puppet_proxy_candidate.h"
#include <iomanip>
#include <set>

static void bridgeRequire(bool value,const char*message){if(!value)throw std::runtime_error(message);}
static std::string bridgeText(const std::filesystem::path&p){std::ifstream in(p,std::ios::binary);bridgeRequire(bool(in),"Read-only source open");return {std::istreambuf_iterator<char>(in),{}};}
static std::string bridgeJson(const std::string&s){std::string out="\"";for(unsigned char c:s){if(c=='"'||c=='\\'){out+='\\';out+=char(c);}else if(c<32){const char*h="0123456789abcdef";out+="\\u00";out+=h[c>>4];out+=h[c&15];}else out+=char(c);}return out+'"';}
static std::string bridgeSnapshot(const MaterialObjectStoreCandidate::Object&o){
 std::ostringstream out(std::ios::binary);auto put=[&](const auto&v){out.write(reinterpret_cast<const char*>(&v),sizeof(v));};
 for(auto v:o.forwarding.values)put(v);
 for(const auto&v:o.parameterState->material.parameters.values){put(v.created);put(v.value.numeric.typeAndMetadata);put(v.value.numeric.integer);for(auto b:v.value.numeric.vector)put(b);put(v.matrixIdentity);const bool matrix=bool(v.matrix);put(matrix);if(v.matrix)for(auto b:*v.matrix)put(b);size_t n=v.name.size();put(n);out<<v.name;n=v.value.text.size();put(n);out<<v.value.text;}
 return out.str();
}
struct ObservedBridgeFactory final:MaterialPuppetFunctionProxyFactoryCandidate {
 std::vector<std::string>requested,unsupported;
 MaterialProxyInstanceCandidate*create(const std::string&name)override{requested.push_back(name);auto*p=MaterialPuppetFunctionProxyFactoryCandidate::create(name);if(!p)unsupported.push_back(name);return p;}
};
static std::string bridgeStrings(const std::vector<std::string>&values){std::string out="[";for(size_t i=0;i<values.size();++i){if(i)out+=',';out+=bridgeJson(values[i]);}return out+']';}
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"Owned scene/material integration diagnostic x64");
 bridgeRequire(argc==4,"session.dmx game/usermod fresh-output.jsonl");
 const std::filesystem::path output=std::filesystem::absolute(argv[3]);
 bridgeRequire(!std::filesystem::exists(output),"Diagnostic output must be fresh");
 std::ofstream report(output,std::ios::binary);bridgeRequire(bool(report),"Diagnostic output open");
 auto emit=[&](const std::string&s){report<<s<<'\n';report.flush();bridgeRequire(bool(report),"Diagnostic report write");};
 const std::filesystem::path mod=std::filesystem::absolute(argv[2]),game=mod.parent_path();
 std::set<std::filesystem::path>readPaths{std::filesystem::absolute(argv[1]),mod/"gameinfo.txt"};
 const auto mounts=gameinfoMountsCandidate(bridgeText(mod/"gameinfo.txt"),mod,game);
 auto graph=loadGraph(argv[1]);const auto root=sessionRenderRootCandidate(graph);
 RetainedSessionSceneCandidate retained(std::move(graph),root,mounts,2,true,true);
 unsigned frames=0,bindings=0,initialized=0,evaluated=0,refused=0,resourceOverrideCases=0;
 std::map<std::pair<uint32_t,size_t>,VtfRgbaImageCandidate>firstImages;
 int32_t currentTick=0;
 retained.setSceneDecorationCandidate([&](const Graph&g,const SessionRenderSceneCandidate&scene,const auto&,const auto&,PreparedSessionSceneCandidate&prepared){
  for(size_t model=0;model<scene.models.size();++model){
   const auto&instance=scene.models[model];const auto context=sessionModelMaterialProxyContextCandidate(g,instance);
   const auto partition=partitionSessionMaterialAlphaCandidate(readSessionMaterialOverridesCandidate(g,instance.id));
   for(size_t slot=0;slot<prepared.materialInputs.at(model).materials.size();++slot){
    const auto&material=prepared.materialInputs.at(model).materials[slot];const auto file=material.source.vmt;
    bridgeRequire(std::filesystem::is_regular_file(file),"Actual resolved VMT required");readPaths.insert(file);
    std::string identity;
    for(const auto&mount:mounts){auto relative=file.lexically_normal().lexically_relative((mount/"materials").lexically_normal());if(relative.empty()||relative.is_absolute())continue;bool outside=false;for(const auto&part:relative)outside|=part=="..";if(!outside){identity=relative.generic_string();break;}}
    bridgeRequire(!identity.empty(),"Resolved VMT identity outside renderer mounts");
    const auto canonical=normalizeSessionMaterialNameCandidate(identity);
    const auto&firstImage=firstImages.emplace(std::make_pair(instance.id,slot),material.image).first->second;
    const bool samePixels=firstImage.width==material.image.width&&firstImage.height==material.image.height&&firstImage.rgba==material.image.rgba;
    emit("{\"kind\":\"cache_binding\",\"tick\":"+std::to_string(currentTick)+",\"model\":"+std::to_string(instance.id)+",\"slot\":"+std::to_string(slot)+",\"identity\":"+bridgeJson(identity)+",\"cpu_input_identity\":"+bridgeJson(std::to_string(reinterpret_cast<uintptr_t>(&prepared.materialInputs.at(model))))+",\"base_texture\":"+bridgeJson(material.source.baseTexture.string())+",\"base_width\":"+std::to_string(material.image.width)+",\"base_height\":"+std::to_string(material.image.height)+",\"base_pixels_equal_first\":"+(samePixels?"true":"false")+",\"base_pixel_storage_identity\":"+bridgeJson(std::to_string(reinterpret_cast<uintptr_t>(material.image.rgba.data())))+"}");
    bool resourceOverrides=false;std::vector<std::string>overrideNames;for(const auto&r:partition.resources)if(r.material==canonical){resourceOverrides=true;for(const auto&p:r.parameters)overrideNames.push_back(p.first+":type"+std::to_string(p.second.type));}
    if(resourceOverrides)++resourceOverrideCases;
    std::string declared="[";bool first=true;
    for(const auto&block:material.source.syntax.children)if(vmtAsciiLowerCandidate(block.key)=="proxies")for(const auto&p:block.children){if(!first)declared+=',';first=false;declared+=bridgeJson(p.key);}declared+=']';
    for(unsigned profile=0;profile<2;++profile){
     ++bindings;MaterialObjectStoreCandidate store;MaterialObjectStoreCandidate::Source source;uintptr_t id=0;
     bridgeRequire(store.createMaterial(identity,source,id),"Controlled material create");
     MaterialObjectParameterOptionsCandidate options;
     // These deliberately different profiles expose sensitivity. Neither is
     // presented as the current original engine's global configuration.
     options.hardwareLevel=profile?95:92;options.environment.condition={profile?1:0,profile?1:0,profile?3:0,profile?1u:0u};
     options.settings.matPhong=profile?1:0;options.settings.toolsSwitchPresent=profile!=0;
     auto observed=std::make_shared<ObservedBridgeFactory>();options.proxyFactory=observed;
     options.provider=[&](const std::string&logical)->std::optional<std::string>{
      // Match the renderer's existing loose-file priority; do not silently
      // replace it with a provider having different implicit archive rules.
      const auto key=sourceMaterialKeyValuesPathCandidate(logical);
      for(const auto&mount:mounts){const auto p=mount/key;if(std::filesystem::is_regular_file(p)){readPaths.insert(p);return bridgeText(p);}}return {};
     };
     MaterialObjectParameterAttemptCandidate init;bool ok=false,eval=false,unchanged=true;size_t writes=0,parameters=0,proxyCount=0;std::string shader,reason;
     try{
      ok=store.initializeParameters(id,options,{},&init);
      if(ok){++initialized;const auto*o=store.get(id);parameters=o->parameterCount;proxyCount=o->proxyState?o->proxyState->size():0;shader=o->parameterState->material.shader;const auto before=bridgeSnapshot(*o);
       if(resourceOverrides){++refused;reason="base VMT initialized only; typed model-resource override transport not connected";}
       else{MaterialProxyEvaluationInputsCandidate inputs;inputs.modelContext=context;
       // Clock/shared missing-var state is intentionally absent. A proxy
       // requiring either must refuse, never consume guessed time or zeroes.
       MaterialProxyEvaluationAttemptCandidate attempt;eval=store.evaluateProxies(id,{},&attempt,inputs);writes=attempt.writes.size();unchanged=bridgeSnapshot(*store.get(id))==before;
       if(eval)++evaluated;else{++refused;reason="proxy bind refused with actual context; clock/shared cache absent";bridgeRequire(unchanged&&!writes,"Failed bind must roll back");}}
      }else{++refused;reason="actual source CPU initialization refused";}
     }catch(const std::exception&e){++refused;reason=std::string("diagnostic exception: ")+e.what();}
     emit("{\"kind\":\"material\",\"tick\":"+std::to_string(currentTick)+",\"model\":"+std::to_string(instance.id)+",\"asset\":"+bridgeJson(instance.placement.asset)+",\"slot\":"+std::to_string(slot)+",\"vmt\":"+bridgeJson(file.string())+",\"identity\":"+bridgeJson(identity)+",\"authored_shader\":"+bridgeJson(material.source.syntax.key)+",\"proxies\":"+declared+",\"profile\":"+std::to_string(profile)+",\"context_known\":"+(context?"true":"false")+",\"resource_override\":"+(resourceOverrides?"true":"false")+",\"override_names\":"+bridgeStrings(overrideNames)+",\"initialized\":"+(ok?"true":"false")+",\"evaluated\":"+(eval?"true":"false")+",\"unchanged\":"+(unchanged?"true":"false")+",\"writes\":"+std::to_string(writes)+",\"parameters\":"+std::to_string(parameters)+",\"proxy_instances\":"+std::to_string(proxyCount)+",\"requested_proxies\":"+bridgeStrings(observed->requested)+",\"unsupported_proxies\":"+bridgeStrings(observed->unsupported)+",\"init_warnings\":"+std::to_string(init.warnings)+",\"selected_shader\":"+bridgeJson(shader)+",\"reason\":"+bridgeJson(reason)+"}");
    }
   }
  }
 });
 for(int32_t tick:{50000,50167,50333}){currentTick=tick;auto prepared=retained.sample(tick,512,512);++frames;emit("{\"kind\":\"frame\",\"tick\":"+std::to_string(tick)+",\"gpu_material_slots\":"+std::to_string(prepared.upload.materials.size())+",\"vertices\":"+std::to_string(prepared.upload.vertices.size())+",\"cache_entries\":"+std::to_string(retained.materialCount())+",\"cache_hits\":"+std::to_string(retained.materialHits())+"}");}
 for(const auto&p:readPaths)emit("{\"kind\":\"read_source\",\"path\":"+bridgeJson(p.string())+"}");
 emit("{\"kind\":\"summary\",\"frames\":"+std::to_string(frames)+",\"bindings\":"+std::to_string(bindings)+",\"initialized\":"+std::to_string(initialized)+",\"evaluated\":"+std::to_string(evaluated)+",\"refused\":"+std::to_string(refused)+",\"resource_override_cases\":"+std::to_string(resourceOverrideCases)+",\"profile_is_original_active_state\":false,\"gpu_or_visual_test\":false}");
 std::cout<<"PASS actual_scene_frames="<<frames<<" actual_material_bindings="<<bindings<<" initialized="<<initialized<<" evaluated="<<evaluated<<" refused="<<refused<<" controlled_profiles_only=1 gpu_test=0\n";return 0;
}catch(const std::exception&e){std::cerr<<"Scene/material diagnostic failed: "<<e.what()<<'\n';return 1;}}
