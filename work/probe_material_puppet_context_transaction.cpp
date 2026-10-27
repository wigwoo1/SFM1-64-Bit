#define SFM64_OBJECT_PARAMETER_MAIN puppetTransactionPriorMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#include "material_weapon_invis_puppet_proxy_candidate.h"
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_model_material_proxy_context_candidate.h"
#include "live_session_scene_candidate.h"
#include "session_render_root_candidate.h"
using PuppetStore=MaterialObjectStoreCandidate;
static Graph puppetGraph(){
 Graph g;g.strings={"DmeGameModel","model","modelName"};
 for(unsigned i=0;i<2;++i){Element e{};e.type=0;e.name=1;e.id[0]=char(i+1);Value v;v.text="models/"+std::string(128,'a')+".mdl";e.attributes.push_back({2,5,{v}});g.elements.push_back(std::move(e));}return g;
}
static MaterialModelProxyBindContextCandidate checkedContext(const Graph&g,uint32_t model){
 auto source=readSessionModelProxyContextCandidate(g,model);auto c=MaterialModelProxyBindContextCandidate::checked(source,model,g.elements.at(model).id,source.asset());gate(c.has_value(),"Checked test context");return *c;
}
static SourceMaterialPatchNodeCandidate puppetRoot(uint32_t kind=0,uint32_t bits=0x3e800000,bool mixed=false,bool observe=false){
 SourceMaterialPatchNodeCandidate root;root.key="UnlitTwoTexture";root.children={{"$cloakpassenabled",2,"",0},{"$before",3,"",0x3e800000},{"$drive",3,"",0x3f000000}};
 SourceMaterialPatchNodeCandidate v;v.key="$cloakfactor";
 switch(kind){case 0:v.kind=3;v.bits=bits;break;case 1:v.kind=1;v.text="retained-"+std::string(80,'x');break;case 2:v.kind=1;v.text="[0.25 -0.5 1]";break;case 4:v.kind=2;v.bits=bits;break;case 6:break;case 7:v.kind=1;v.text="[1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]";break;default:gate(false,"Seed kind");}
 if(kind!=6)root.children.push_back(v);
 SourceMaterialPatchNodeCandidate p;p.key="Proxies";
 if(mixed){SourceMaterialPatchNodeCandidate e;e.key="Equals";e.children={{"resultVar",1,"$before"},{"srcVar1",1,"$drive"}};p.children.push_back(e);}
 if(observe)p.children.push_back({"ObservePuppet"});
 // Authored resultVar must be ignored by the actual fixed-result weapon Init.
 SourceMaterialPatchNodeCandidate w;w.key="weapon_invis";w.children={{"resultVar",1,"$before"}};p.children.push_back(w);root.children.push_back(p);return root;
}
static std::string storeSnapshot(PuppetStore&s,uintptr_t id){
 auto*p=s.get(id);gate(p&&p->parameterState,"Snapshot initialized material");std::ostringstream b;putValues(b,p->parameterState->material);for(auto v:p->forwarding.values)put(b,v);return b.str();
}
static uintptr_t createPuppetMaterial(PuppetStore&s,const SourceMaterialPatchNodeCandidate&root,const std::shared_ptr<MaterialProxyFactoryCandidate>&factory){
 PuppetStore::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(root);uintptr_t id=0;MaterialObjectParameterOptionsCandidate options;options.proxyFactory=factory;gate(s.createMaterial("puppet/material",source,id)&&s.initializeParameters(id,options),"Material init");return id;
}
struct PuppetObserverFactory:MaterialPuppetFunctionProxyFactoryCandidate {
 std::function<bool(MaterialProxyEvaluationCandidate&)>hook;MaterialProxyBindingCandidate before,cloak;unsigned created=0,destroyed=0;
 struct Observer:MaterialProxyInstanceCandidate{PuppetObserverFactory&f;explicit Observer(PuppetObserverFactory&value):f(value){}bool initialize(const MaterialProxyContextCandidate&c,const SourceMaterialPatchNodeCandidate&)override{f.before=c.find("$before");f.cloak=c.find("$cloakfactor");return f.before.valid()&&f.cloak.valid();}bool evaluate(MaterialProxyEvaluationCandidate&frame)override{return !f.hook||f.hook(frame);}};
 MaterialProxyInstanceCandidate*create(const std::string&name)override{auto*p=name=="ObservePuppet"?new Observer(*this):MaterialPuppetFunctionProxyFactoryCandidate::create(name);if(p)++created;return p;}
 void destroy(MaterialProxyInstanceCandidate*p)noexcept override{++destroyed;delete p;}
};
static int safety(){
 static_assert(sizeof(void*)==8,"Owned material transaction x64");unsigned n=0;auto check=[&](bool b){if(!b)throw std::runtime_error("Puppet safety assertion="+std::to_string(n));++n;};
 auto graph=puppetGraph();auto known=checkedContext(graph,0),other=checkedContext(graph,1);SessionModelProxyContextCandidate unknown;
 check(!MaterialModelProxyBindContextCandidate::checked(unknown,0,graph.elements[0].id,known.source().asset()));
 check(!MaterialModelProxyBindContextCandidate::checked(known.source(),1,graph.elements[0].id,known.source().asset()));
 auto wrongGuid=graph.elements[0].id;wrongGuid[3]=7;check(!MaterialModelProxyBindContextCandidate::checked(known.source(),0,wrongGuid,known.source().asset()));check(!MaterialModelProxyBindContextCandidate::checked(known.source(),0,graph.elements[0].id,"models/wrong.mdl"));
 auto factory=std::make_shared<PuppetObserverFactory>();PuppetStore store;auto id=createPuppetMaterial(store,puppetRoot(7,0,true,true),factory);auto*root=store.get(id)->source.get();auto savedRoot=*root;const auto before=storeSnapshot(store,id);auto*state=store.get(id)->parameterState.get();MaterialProxyEvaluationAttemptCandidate attempt;MaterialProxyEvaluationInputsCandidate inputs;
 auto unchanged=[&](){return storeSnapshot(store,id)==before&&store.get(id)->parameterState.get()==state&&attempt.writes.empty()&&!attempt.evaluated&&!store.get(id)->evaluationPending&&materialObjectParameterTreeEqualCandidate(*root,savedRoot);};
 check(!store.evaluateProxies(id,{},&attempt,inputs)&&unchanged());inputs.modelContext=MaterialModelProxyBindContextCandidate{};check(!store.evaluateProxies(id,{},&attempt,inputs)&&unchanged());inputs.modelContext=known;
 check(!store.evaluateProxies(id,[](auto phase,size_t){return phase!=PuppetStore::Allocation::evaluation;},&attempt,inputs)&&unchanged());
 check(!store.evaluateProxies(id,[](auto phase,size_t){return phase!=PuppetStore::Allocation::evaluationCommit;},&attempt,inputs)&&unchanged());
 factory->hook=[](auto&)->bool{throw std::runtime_error("Service failure");};check(!store.evaluateProxies(id,{},&attempt,inputs)&&unchanged());factory->hook={};
 unsigned allocationFailures=0;bool success=false;
 for(int64_t step=0;step<4096;++step){auto balance=heapNew-heapDelete;failAfter=step;const bool ok=store.evaluateProxies(id,{},&attempt,inputs);failAfter=-1;if(ok){success=true;break;}++allocationFailures;check(heapNew-heapDelete==balance&&unchanged());}
 check(success&&allocationFailures>0&&attempt.evaluated&&attempt.writes.size()==1&&attempt.writes[0].operation==1&&attempt.writes[0].arguments[0]==0x3f000000);
 auto after=storeSnapshot(store,id);auto retainedCloak=factory->cloak.read();check(retainedCloak&&retainedCloak->matrix&&retainedCloak->matrixIdentity);
 // Reset the committed mixed result, then inspect staged reads while public
 // bindings still see the previous value. The entire frame owns its context.
 check(store.uncacheParameters(id)&&!factory->cloak.valid());MaterialObjectParameterOptionsCandidate options;options.proxyFactory=factory;check(store.initializeParameters(id,options));
 auto stale=factory->cloak;bool observed=false;inputs.modelContext=known;
 factory->hook=[&](auto&frame){observed=true;check(frame.modelContextModel()==std::optional<uint32_t>(0)&&frame.verifiedPuppetWithoutGameEntity());check(frame.read(factory->before)->value.numeric.vector[0]==0x3f000000&&factory->before.read()->value.numeric.vector[0]==0x3e800000);inputs.modelContext=other;check(frame.modelContextModel()==std::optional<uint32_t>(0));check(!store.removeAndDelete(id)&&!store.uncacheParameters(id)&&!store.evaluateProxies(id));return true;};
 check(store.evaluateProxies(id,{},&attempt,inputs)&&observed&&attempt.writes.size()==1);factory->hook={};
 // Policy callbacks happen before proxy callbacks; they cannot replace the
 // snapshot of the draw that entered this evaluation call.
 inputs.modelContext=known;factory->hook=[&](auto&frame){check(frame.modelContextModel()==std::optional<uint32_t>(0));return true;};
 check(store.evaluateProxies(id,[&](auto phase,size_t){if(phase==PuppetStore::Allocation::evaluation)inputs.modelContext=other;return true;},&attempt,inputs));factory->hook={};
 check(store.uncacheParameters(id)&&!stale.valid()&&store.initializeParameters(id,options));
 factory->hook=[&](auto&frame){check(!frame.read(stale));return false;};const auto stable=storeSnapshot(store,id);check(!store.evaluateProxies(id,{},&attempt,inputs)&&storeSnapshot(store,id)==stable&&attempt.writes.empty());factory->hook={};
 PuppetStore second;auto secondFactory=std::make_shared<PuppetObserverFactory>();auto secondId=createPuppetMaterial(second,puppetRoot(1,0,true,true),secondFactory);MaterialProxyEvaluationInputsCandidate secondInput;secondInput.modelContext=other;auto foreign=secondFactory->cloak;
 factory->hook=[&](auto&frame){check(!frame.read(foreign));return second.evaluateProxies(secondId,{},nullptr,secondInput);};check(store.evaluateProxies(id,{},nullptr,inputs));factory->hook={};check(second.removeAndDelete(secondId)&&secondFactory->created==secondFactory->destroyed);
 check(store.removeAndDelete(id)&&factory->created==factory->destroyed&&!factory->cloak.valid());
 // Binding revalidation rejects a saved scene after its source identity or
 // factory category changes, while captured values remain safely owned.
 auto source=readSessionModelProxyContextCandidate(graph,0);SessionRenderModelCandidate model{};model.id=0;model.placement.asset=source.asset();model.proxyContext=source;check(sessionModelMaterialProxyContextCandidate(graph,model).has_value());graph.elements[0].attributes[0].values[0].text="*17";check(!sessionModelMaterialProxyContextCandidate(graph,model));graph=puppetGraph();graph.elements[0].id[2]=9;check(!sessionModelMaterialProxyContextCandidate(graph,model));
 // Old general factory still refuses weapon_invis. A shader lacking the fixed
 // result must fail Init; caller-authored resultVar cannot redirect this proxy.
 {PuppetStore s;PuppetStore::Source tree=std::make_unique<SourceMaterialPatchNodeCandidate>(puppetRoot());uintptr_t x=0;MaterialObjectParameterOptionsCandidate opt;opt.proxyFactory=std::make_shared<MaterialFunctionProxyFactoryCandidate>();check(s.createMaterial("generic/refusal",tree,x)&&!s.initializeParameters(x,opt)&&!s.get(x)->parameterState&&s.removeAndDelete(x));}
 {PuppetStore s;auto tree=puppetRoot();tree.key="Wireframe";tree.children.erase(std::remove_if(tree.children.begin(),tree.children.end(),[](auto&v){return v.key=="$cloakfactor";}),tree.children.end());PuppetStore::Source p=std::make_unique<SourceMaterialPatchNodeCandidate>(tree);uintptr_t x=0;MaterialObjectParameterOptionsCandidate opt;opt.proxyFactory=std::make_shared<MaterialPuppetFunctionProxyFactoryCandidate>();check(s.createMaterial("fixed/result",p,x)&&!s.initializeParameters(x,opt)&&!s.get(x)->parameterState&&s.removeAndDelete(x));}
 std::cout<<"PASS puppet_transaction_safety_checks="<<n<<" actual_allocation_failures="<<allocationFailures<<'\n';return 0;
}
static int corpus(int argc,char**argv){
 gate(argc==4&&!std::ifstream(argv[3]).good(),"absent-DLL selected-query fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);gate(bool(in)&&bool(out),"Corpus IO");auto graph=puppetGraph();MaterialProxyEvaluationInputsCandidate inputs;inputs.modelContext=checkedContext(graph,0);auto factory=std::make_shared<MaterialPuppetFunctionProxyFactoryCandidate>();uint32_t index=0,accepted=0,refused=0;std::array<uint32_t,11>q{};
 while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
  gate((q[0]&3)==3&&!(q[0]&4),"Measured puppet no-entity subset");auto kind=q[1]&15u;PuppetStore s;auto root=puppetRoot(kind,q[10]);auto rootBefore=root;PuppetStore::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(root);uintptr_t id=0;gate(s.createMaterial("puppet/material",source,id),"Corpus material creation");MaterialObjectParameterOptionsCandidate options;options.proxyFactory=factory;const bool initialized=s.initializeParameters(id,options);
  const bool nonfinite=kind==0&&(q[10]&0x7fffffffu)>=0x7f800000u;
  uint32_t actual=UINT32_MAX;std::string before,after;
  if(nonfinite){
   // Preserve the existing material-tree admission gate. The original proxy
   // fixture accepts arbitrary preconstructed variables; this real store
   // intentionally refuses nonfinite authored literal trees before proxy Init.
   gate(!initialized&&!s.get(id)->parameterState&&s.get(id)->flags==16&&materialObjectParameterTreeEqualCandidate(*s.get(id)->source,rootBefore),"Existing nonfinite-root refusal preserved");std::ostringstream b;putTree(b,rootBefore);before=b.str();std::ostringstream a;putTree(a,*s.get(id)->source);after=a.str();++refused;
  }else{
   gate(initialized,"Canonical material Init");before=storeSnapshot(s,id);MaterialProxyEvaluationAttemptCandidate a;gate(s.evaluateProxies(id,{},&a,inputs)&&a.evaluated&&a.writes.empty(),"Actual no-write store transaction");after=storeSnapshot(s,id);auto&values=s.get(id)->parameterState->material.parameters.values;auto found=std::find_if(values.begin(),values.end(),[](auto&v){return sourceMaterialPatchLowerCandidate(v.name)=="$cloakfactor";});gate(found!=values.end(),"Fixed result");actual=found->value.numeric.typeAndMetadata&15u;
   // Existing descriptor completion promotes absent Undefined cloakfactor to
   // Float before evaluation. The proxy is checked against initialized values.
   gate(actual==(kind==6?0u:kind),"Canonical initial type coverage");++accepted;
  }
  gate(before==after,"Complete admitted state or refused root preservation");put(out,index++);put(out,kind);put(out,actual);put(out,initialized);put(out,0);put(out,uint32_t(before.size()));out.write(before.data(),before.size());put(out,uint32_t(after.size()));out.write(after.data(),after.size());gate(s.removeAndDelete(id),"Corpus cleanup");
 }
 gate(in.eof()&&in.gcount()==0&&index,"Complete selected corpus");out.flush();gate(bool(out),"Complete output");std::cout<<"PASS puppet_transaction_cases="<<index<<" evaluated_zero_writes="<<accepted<<" init_refused="<<refused<<" preserved_records="<<index<<'\n';return 0;
}
static int benchmark(const char*path){
 auto graph=loadGraph(path);RetainedShotTimelineCandidate timeline(graph,sessionRenderRootCandidate(graph));unsigned accepted=0,unknown=0;
 auto factory=std::make_shared<MaterialPuppetFunctionProxyFactoryCandidate>();
 for(int32_t tick:{50000,50167,50333}){
  auto scene=evaluateLiveSessionSceneCandidate(timeline,tick,512,512).scene;
  for(const auto&model:scene.models){PuppetStore store;auto id=createPuppetMaterial(store,puppetRoot(model.id%2?1:7,0,true),factory);const auto before=storeSnapshot(store,id);MaterialProxyEvaluationInputsCandidate inputs;inputs.modelContext=sessionModelMaterialProxyContextCandidate(timeline.graph(),model);MaterialProxyEvaluationAttemptCandidate attempt;
   const bool evaluated=store.evaluateProxies(id,{},&attempt,inputs);
   if(inputs.modelContext){gate(evaluated&&attempt.evaluated&&attempt.writes.size()==1&&attempt.writes[0].arguments[0]==0x3f000000,"Bound benchmark mixed frame");++accepted;}
   else{gate(!evaluated&&!attempt.evaluated&&attempt.writes.empty()&&storeSnapshot(store,id)==before,"Unknown benchmark context rollback");++unknown;}
   gate(store.removeAndDelete(id),"Benchmark material cleanup");
  }
 }
 std::cout<<"PASS benchmark_puppet_transaction_frames=3 accepted="<<accepted<<" unknown_refused="<<unknown<<" shader_fixture=controlled actual_asset_materials=NOT_TESTED\n";return 0;
}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"Owned independent material-context transactions");if(argc==2&&std::string(argv[1])=="--safety")return safety();if(argc==3&&std::string(argv[1])=="--benchmark")return benchmark(argv[2]);return corpus(argc,argv);}catch(const std::exception&e){failAfter=-1;std::cerr<<e.what()<<'\n';return 1;}}
