#define main precedingSingleMain
#include "test_material_store_session_scope.cpp"
#undef main
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_model_proxy_context_candidate.h"
#include "session_material_overrides_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "material_session_file_runtime_candidate.h"
#include "material_weapon_invis_puppet_proxy_candidate.h"
#include <cmath>
#include <filesystem>
struct PilotFactory:MaterialPuppetFunctionProxyFactoryCandidate{
 size_t created=0,destroyed=0;std::vector<std::string>names;
 MaterialProxyInstanceCandidate*create(const std::string&name)override{names.push_back(name);auto*p=MaterialPuppetFunctionProxyFactoryCandidate::create(name);if(p)++created;return p;}
 void destroy(MaterialProxyInstanceCandidate*p)noexcept override{++destroyed;delete p;}
};
static uint32_t bitsPilot(float f){uint32_t b;std::memcpy(&b,&f,4);return b;}
static float floatPilot(uint32_t b){float f;std::memcpy(&f,&b,4);return f;}
static std::string readPilot(const std::filesystem::path&path){std::ifstream f(path,std::ios::binary);need(bool(f),"read-only pilot input");std::string s((std::istreambuf_iterator<char>(f)),{});need(s.size()<=1u<<24,"pilot read extent");return s;}
static uint64_t hashPilot(const std::string&s){uint64_t h=14695981039346656037ull;for(unsigned char c:s){h^=c;h*=1099511628211ull;}return h;}
static int testPilot(const char*session,const char*gameDirectory,const char*destination){
 need(!std::ifstream(destination).good(),"fresh private evidence output");std::ofstream out(destination,std::ios::binary);need(bool(out),"pilot output");unsigned checks=0;auto check=[&](bool b,const char*s){need(b,s);++checks;};
 const std::filesystem::path game=gameDirectory;auto info=game/"usermod/gameinfo.txt";auto infoText=readPilot(info);out<<"F\t"<<info.generic_string()<<'\t'<<infoText.size()<<'\t'<<hashPilot(infoText)<<'\n';auto mounts=gameinfoMountsCandidate(infoText,game/"usermod",game);
 std::map<std::string,std::optional<std::string>>files;
 SourceMaterialKeyValuesSourceCandidate provider=[&](const std::string&request)->std::optional<std::string>{auto key=sourceMaterialKeyValuesPathCandidate(request);auto previous=files.find(key);if(previous!=files.end())return previous->second;need(key.size()>=4&&key.substr(key.size()-4)==".vmt","VMT read only");for(const auto&mount:mounts){auto path=mount/key;if(!std::filesystem::is_regular_file(path))continue;auto data=readPilot(path);out<<"F\t"<<path.generic_string()<<'\t'<<data.size()<<'\t'<<hashPilot(data)<<'\n';files.emplace(key,data);return data;}files.emplace(key,std::nullopt);return {};};
 // Actual category distinction: a supplied-source material must not intercept
 // creation of a same-named filename resource.
 // Fill the dictionary's reported initial capacity so the tree policy
 // genuinely reaches a growth allocation; an empty insertion allocates none.
 for(auto stage:{Store::Allocation::symbols,Store::Allocation::object,Store::Allocation::registry,Store::Allocation::tree})for(bool throwing:{false,true}){Store fixture;Store::Id seed=0;const auto capacity=MaterialDictionaryTreeCandidate{}.capacity();for(size_t i=0;i<capacity;++i)need(fixture.createFileMaterial("fixture/"+std::to_string(i),"fixture",seed),"tree capacity fixture");Store::Id unchanged=777;const auto symbols=fixture.symbolCount(),count=fixture.size();check(!fixture.createFileMaterial("fixture/file","fixture",unchanged,[&](Store::Allocation kind,size_t){if(kind==stage){if(throwing)throw std::bad_alloc();return false;}return true;})&&unchanged==777&&fixture.size()==count&&fixture.symbolCount()==symbols&&fixture.valid(),"filename creation allocation policy refusal preserves registry/output");}

 {Store fixture;auto root=std::make_unique<SourceMaterialPatchNodeCandidate>();root->key="VertexLitGeneric";Store::Id supplied=0;need(fixture.createMaterial("category/item",root,supplied),"supplied category fixture");MaterialObjectParameterOptionsCandidate options;options.provider=[](const std::string&path)->std::optional<std::string>{if(path=="materials/category/item.vmt")return "VertexLitGeneric { \"$alpha\" \".25\" }";return {};};MaterialSessionFileRuntimeCandidate runtime(fixture,options);auto file=runtime.findMaterial("CATEGORY\\ITEM.vmt");check(file&&*file!=supplied&&fixture.find("category/item",false)==*file&&fixture.find("category/item",true)==supplied,"same name different source categories");check(!fixture.get(*file)->source&&fixture.get(*file)->references.load()==0&&!fixture.get(*file)->parameterState,"file publication does not invent init or AddRef");check(runtime.initialize(*file)&&!runtime.initialize(supplied),"explicit filename initialization");check(runtime.findMaterial("category/item")==file&&fixture.get(*file)->references.load()==0,"cached lookup no added reference");check(!runtime.findMaterial("absent")&&!runtime.findMaterial("{procedural}")&&!runtime.findMaterial(std::string(4096,'x')),"unavailable request remains unavailable");}
 Store store;auto factory=std::make_shared<PilotFactory>();MaterialObjectParameterOptionsCandidate options;options.provider=provider;options.proxyFactory=factory;MaterialSessionFileRuntimeCandidate runtime(store,options);MaterialSessionStoreBindingsCandidate bindings(store,[&](auto name){return runtime.findMaterial(name);});
 auto graph=loadGraph(session);std::vector<uint32_t>materials,models;
 for(uint32_t i=0;i<graph.elements.size();++i){const auto&e=graph.elements[i];if(graph.strings.at(e.type)=="DmeMaterial"){auto*a=namedAttribute(graph,e,"mtlName");if(a&&a->type==5&&a->values.size()==1&&normalizeSessionMaterialNameCandidate(a->values[0].text)=="models/effects/pyro/pilotlight"){check(bindings.define(i)&&bindings.setName(i,a->values[0].text)&&bindings.clearDirty(i),"real session element binding");materials.push_back(i);}}
 }
 check(!materials.empty(),"benchmark pilot elements exist");Store::Id id=0;
 for(auto material:materials){auto acquired=bindings.acquire(material);check(acquired&&*acquired,"real file-backed owned material acquisition");if(!id)id=*acquired;check(*acquired==id,"same material shared by distinct session elements");out<<"M\t"<<material<<'\t'<<*acquired<<'\n';}
 check(store.get(id)->references.load()==int(materials.size())&&!store.get(id)->parameterState,"retained resource exists before initialization");
 check(runtime.initialize(id)&&store.get(id)->parameterState&&store.get(id)->proxyState&&store.get(id)->proxyState->size()==7,"actual file full seven-proxy CPU initialization");check(factory->names==std::vector<std::string>{"Sine","Sine","Equals","Equals","TextureTransform","TextureScroll","weapon_invis"},"actual declaration/factory order");
 out<<"S\t"<<store.get(id)->parameterState->material.shader<<'\t'<<store.get(id)->parameterCount<<'\t'<<store.get(id)->proxyState->size()<<'\n';
 check(store.get(id)->parameterState->material.shader=="UnlitTwoTexture_DX9"&&store.get(id)->parameterCount==24,"actual pilot alias fallback and parameter schema");
 for(uint32_t i=0;i<graph.elements.size();++i){const auto&e=graph.elements[i];if(graph.strings.at(e.type)!="DmeGameModel")continue;auto*a=namedAttribute(graph,e,"materials");if(!a||a->type!=15)continue;for(const auto&v:a->values)if(std::find(materials.begin(),materials.end(),v.reference)!=materials.end()){models.push_back(i);break;}}
 check(!models.empty(),"benchmark models own pilot material");auto source=readSessionModelProxyContextCandidate(graph,models.front());auto context=MaterialModelProxyBindContextCandidate::checked(source,models.front(),graph.elements[models.front()].id,source.asset());check(context&&context->verifiedPuppetWithoutEntity(),"identity-checked real benchmark puppet context");
 for(auto model:models){auto value=readSessionModelProxyContextCandidate(graph,model);check(value.knownOrdinaryIfmPuppet(),"all pilot model source contexts are ordinary puppets");out<<"O\t"<<model<<'\t'<<value.asset()<<'\n';}
 const auto before=snapshot(store,id);const auto*original=store.get(id)->parameterState.get();const auto originalCloak=find(*original,"$cloakfactor").value.numeric.vector[0];MaterialProxyEvaluationAttemptCandidate attempt;
 MaterialProxyEvaluationInputsCandidate input;input.currentTimeBits=bitsPilot(.125f);
 check(!store.evaluateProxies(id,{},&attempt,input)&&attempt.writes.empty()&&store.get(id)->parameterState.get()==original&&snapshot(store,id)==before,"unknown game entity context refuses full frame without prefix publication");input.modelContext=context;input.currentTimeBits.reset();check(!store.evaluateProxies(id,{},&attempt,input)&&snapshot(store,id)==before,"missing clock refuses, no invented timezero");
 for(float time:{0.f,.125f,.5f,1.f,5.f,17.25f,-.125f}){
  input.currentTimeBits=bitsPilot(time);check(store.evaluateProxies(id,{},&attempt,input)&&attempt.evaluated&&attempt.writes.size()==6,"full ordered proxy frame");const auto&state=*store.get(id)->parameterState;
  const auto&pilot=find(state,"$pilot");const auto&pilot2=find(state,"$pilot2");const auto&temp=find(state,"$temp");const auto&cloak=find(state,"$cloakfactor");const auto&base=find(state,"$basetexturetransform");const auto&second=find(state,"$texture2transform");
  const auto wave=[](float t,float period,float lo,float hi){const double angle=double(t)*6.283185307179586476925286766559/double(period);return lo+float((std::sin(angle)+1.)*.5)*float(hi-lo);};
  check(std::abs(floatPilot(pilot.value.numeric.vector[0])-wave(time,.1f,-.02f,.02f))<2e-6f&&std::abs(floatPilot(pilot2.value.numeric.vector[0])-wave(time,7.f,.005f,-.005f))<2e-6f,"independent sine analytic values (bounded tolerance, not exact native composition)");
  check(temp.value.numeric.vector[0]==pilot2.value.numeric.vector[0]&&temp.value.numeric.vector[1]==pilot.value.numeric.vector[0],"Equals reads preceding Sine writes in same frame");check(base.matrix&&second.matrix,"both texture transforms are actual typed matrices");
  check(std::abs(floatPilot(base.matrix->at(3))-floatPilot(temp.value.numeric.vector[0]))<1e-7f&&std::abs(floatPilot(base.matrix->at(7))-floatPilot(temp.value.numeric.vector[1]))<1e-7f,"translation matrix reflects preceding component writes");
  const auto wrap=[](float value){return value-std::floor(value);};const auto x=float(std::cos(87.*3.14159265358979323846/180.)*double(time)*3.);const auto y=float(std::sin(87.*3.14159265358979323846/180.)*double(time)*3.);
  check(std::abs(floatPilot(second.matrix->at(3))-wrap(x))<2e-6f&&std::abs(floatPilot(second.matrix->at(7))-wrap(y))<2e-6f,"independent time/rate/angle scroll projection");
  check(cloak.value.numeric.vector[0]==originalCloak,"verified puppet weapon_invis branch does not write cloak");
  std::vector<uint32_t>operations;for(const auto&w:attempt.writes)operations.push_back(w.operation);check(operations==std::vector<uint32_t>{1,1,8,8,9,9},"six setters in original declaration order; no invented seventh setter");out<<"T\t"<<bitsPilot(time)<<'\t'<<pilot.value.numeric.vector[0]<<'\t'<<pilot2.value.numeric.vector[0]<<'\t'<<base.matrix->at(3)<<'\t'<<base.matrix->at(7)<<'\t'<<second.matrix->at(3)<<'\t'<<second.matrix->at(7)<<'\t'<<attempt.writes.size()<<'\n';
 }
 // The checked constructor refuses a model/GUID/asset identity mismatch.
 auto wrong=MaterialModelProxyBindContextCandidate::checked(source,models.front()+1,graph.elements[models.front()].id,source.asset());check(!wrong,"foreign model identity cannot produce authorized context");
 auto committed=snapshot(store,id);input.modelContext=MaterialModelProxyBindContextCandidate{};check(!store.evaluateProxies(id,{},&attempt,input)&&snapshot(store,id)==committed&&attempt.writes.empty(),"unknown context after success still rolls back all writes");
 for(auto material:materials)check(bindings.erase(material),"retire actual session element reference");check(store.get(id)->references.load()==0,"all actual pilot references balanced");check(store.uncacheParameters(id)&&factory->created==7&&factory->destroyed==7,"all seven actual proxy instances retired");
 out.flush();need(bool(out),"pilot evidence complete");std::cout<<"PASS actual_pilot_runtime_checks="<<checks<<" session_elements="<<materials.size()<<" session_models="<<models.size()<<" initialized_cpu=1 proxy_instances=7 evaluated_times=7 journal_writes=42 shader=UnlitTwoTexture_DX9 GPU=NOT_CONNECTED native_full_composition=UNVERIFIED\n";return 0;
}
int main(int argc,char**argv){try{need(argc==4,"session game-directory fresh-output");return testPilot(argv[1],argv[2],argv[3]);}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
