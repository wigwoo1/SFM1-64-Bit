#ifdef SFM64_NATIVE_CLIENT_EQUALS
#define SFM64_NATIVE_OBJECT_PARAMETERS
#endif
#define SFM64_OBJECT_PARAMETER_MAIN multiplyPriorParameterMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#ifdef SFM64_NATIVE_CLIENT_EQUALS
#include "material_equals_client_native_fixture.h"
#else
#include "material_multiply_proxy_candidate.h"
#include "material_session_file_runtime_candidate.h"
#include "gameinfo_mounts_candidate.h"
static int multiplySafety();
static int weaponRefusal(const char*gameDirectory);
#endif
static const SourceMaterialPatchNodeCandidate*equalsDeclarations(const MaterialObjectParameterPlanCandidate&plan){auto*p=materialProxySelectedNodeCandidate(plan.finalRoot,plan.state->material.selectedNode);gate(p!=nullptr,"Selected node");auto*r=sourceMaterialPatchFindConstCandidate(*p,"Proxies");return r?r:sourceMaterialPatchFindConstCandidate(plan.finalRoot,"Proxies");}
static void putWrites(std::ostream&out,const std::vector<MaterialProxyWriteCandidate>&writes){put(out,uint32_t(writes.size()));for(const auto&w:writes){put(out,w.index);put(out,w.operation);put(out,w.count);for(auto b:w.arguments)put(out,b);}}
static MaterialObjectParameterPlanCandidate readPrepared(std::istream&in,const SourceMaterialPatchNodeCandidate&root){MaterialObjectParameterPlanCandidate p;p.finalRoot=root;p.state=std::make_unique<MaterialObjectParameterStateCandidate>();auto&m=p.state->material;m.shader=text(in);auto count=word(in);gate(count>=13&&count<=255,"Prepared variable count");for(unsigned i=0;i<count;++i){std::array<uint32_t,28>q{};for(auto&n:q)n=word(in);MaterialTypedDispatchCandidate v;v.created=q[0]!=0;v.value.numeric.typeAndMetadata=q[1];v.value.numeric.integer=q[2];std::copy_n(q.begin()+3,4,v.value.numeric.vector.begin());gate(q[10]<4096&&q[11]<1024,"Prepared text extent");v.value.text.resize(q[10]);v.name.resize(q[11]);in.read(v.value.text.data(),q[10]);in.read(v.name.data(),q[11]);if(q[8]){v.matrix.emplace();std::copy_n(q.begin()+12,16,v.matrix->begin());}v.matrixIdentity=q[9]!=0;m.parameters.values.push_back(std::move(v));}gate(bool(in),"Complete prepared state");return p;}
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_CLIENT_EQUALS
 if(argc==1)return multiplySafety();if(argc==3&&std::string(argv[1])=="--weapon-refusal")return weaponRefusal(argv[2]);static_assert(sizeof(void*)==8,"owned x64 Multiply");
#endif
 gate(argc==5&&!std::ifstream(argv[4]).good(),"client-or-absent query prepared-state fresh-output");std::ifstream in(argv[2],std::ios::binary),prepared(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);gate(bool(in)&&bool(prepared)&&bool(out),"Multiply IO");
#ifdef SFM64_NATIVE_CLIENT_EQUALS
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);MaterialEqualsClientNativeFixture client(argv[1]);unsigned old=0,temp=0;_controlfp_s(&old,0,0);_controlfp_s(&temp,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#endif
 const auto mx=_mm_getcsr();_mm_setcsr(0x1f80);auto count=word(in);gate(count&&count<20000&&word(prepared)==count,"Multiply case count");put(out,count);unsigned frames=0,refused=0,rawCases=0,factoryCases=0;
 for(unsigned ci=0;ci<count;++ci){auto mode=word(in);gate(mode<=1,"factory or raw value mode");auto root=node(in);auto plan=readPrepared(prepared,root);auto*decls=equalsDeclarations(plan);gate(decls,"declarations");
#ifdef SFM64_NATIVE_CLIENT_EQUALS
  client.prepare(*plan.state);std::vector<void*>proxies;bool initialized=true;for(const auto&declaration:decls->children){gate(declaration.key=="Multiply","native name");using Create=void*(__cdecl*)();auto*p=reinterpret_cast<Create>(0x10137010)();gate(p&&*static_cast<uintptr_t*>(p)==0x104ffeac,"actual Multiply factory/vtable");using Init=bool(__thiscall*)(void*,void*,void*);if(!reinterpret_cast<Init>(0x10136c20)(p,&client.material,client.build(declaration))){client.destroy(p);initialized=false;break;}gate(client.materialIdentity(p),"actual material identity");proxies.push_back(p);}
#else
  MaterialObjectStoreCandidate store;uintptr_t id=0;bool initialized=true;MaterialObjectParameterOptionsCandidate options;options.proxyFactory=std::make_shared<MaterialMultiplyPuppetProxyFactoryCandidate>();
  if(!mode){MaterialObjectParameterPlanCandidate own;gate(materialObjectParameterPrepareCandidate(own,"multiply/case",0x10,&root,options),"owned CPU prepare");std::ostringstream a,b;putValues(a,own.state->material);putValues(b,plan.state->material);gate(a.str()==b.str(),"owned initial state equals separately executed original CPU preparation");MaterialObjectStoreCandidate::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(root);gate(store.createMaterial("multiply/case",source,id),"owned material");initialized=store.initializeParameters(id,options);}
  else{for(const auto&d:decls->children){for(const auto*key:{"resultVar","srcVar1","srcVar2"}){const auto*p=sourceMaterialPatchFindConstCandidate(d,key);if(!p||p->text.empty())initialized=false;}}}
#endif
  put(out,ci);put(out,initialized);if(!initialized)++refused;
  if(initialized)for(unsigned frame=0;frame<3;++frame){
#ifdef SFM64_NATIVE_CLIENT_EQUALS
   client.writes.clear();using Bind=void(__thiscall*)(void*,void*);for(auto*p:proxies)reinterpret_cast<Bind>(0x10136f00)(p,nullptr);client.image.verify();putWrites(out,client.writes);auto state=*plan.state;state.material.parameters.values.clear();for(const auto&v:client.variables)state.material.parameters.values.push_back(v.value);
#else
   MaterialObjectParameterStateCandidate state;std::vector<MaterialProxyWriteCandidate>writes;
   if(!mode){MaterialProxyEvaluationAttemptCandidate attempt;gate(store.evaluateProxies(id,{},&attempt)&&attempt.evaluated,"actual owned factory/store frame");writes=attempt.writes;state=*store.get(id)->parameterState;}
   else{state=*plan.state;auto&values=state.material.parameters.values;auto locate=[&](std::string name)->uint32_t{for(size_t i=values.size();i>0;--i)if(sourceMaterialPatchLowerCandidate(values[i-1].name)==sourceMaterialPatchLowerCandidate(name))return uint32_t(i-1);throw std::runtime_error("raw variable not found");};for(const auto&d:decls->children){auto output=sourceMaterialPatchFindConstCandidate(d,"resultVar")->text;int32_t component=-1;auto at=output.find('[');if(at!=std::string::npos){component=int32_t(materialStringIntegerCandidate(output.substr(at+1)));output.resize(at);}const auto index=locate(output);auto a=locate(sourceMaterialPatchFindConstCandidate(d,"srcVar1")->text),b=locate(sourceMaterialPatchFindConstCandidate(d,"srcVar2")->text);auto result=materialMultiplyResultCandidate(values[index].value.numeric,values[a].value.numeric,values[b].value.numeric,component);gate(result.has_value(),"bounded raw result plan");if(result->write){gate(materialTypedNumericWriteCandidate(values[index],result->operation,result->arguments,result->count),"shared checked typed setter");writes.push_back({index,result->operation,result->count,result->arguments});}}*plan.state=state;}
   putWrites(out,writes);
#endif
   putValues(out,state.material);std::array<uint32_t,7>forward{};gate(materialObjectParameterForwardCandidate(forward,state),"forward values");for(auto v:forward)put(out,v);++frames;
  }
#ifdef SFM64_NATIVE_CLIENT_EQUALS
  for(auto i=proxies.rbegin();i!=proxies.rend();++i)client.destroy(*i);client.image.verify();gate(client.blocks.empty(),"native case balanced");
#else
  if(!mode)gate(store.removeAndDelete(id),"owned cleanup");
#endif
  rawCases+=mode;factoryCases+=!mode;
 }
 gate(in.peek()==std::char_traits<char>::eof()&&prepared.peek()==std::char_traits<char>::eof(),"Multiply input tail");out.flush();gate(bool(out),"complete Multiply trace");_mm_setcsr(mx);
#ifdef SFM64_NATIVE_CLIENT_EQUALS
 client.image.verify();_controlfp_s(&temp,old,_MCW_RC|_MCW_PC);std::cout<<"native_proxy_allocations="<<client.allocations<<" releases="<<client.releases<<'\n';
#endif
 std::cout<<"PASS multiply_cases="<<count<<" frames="<<frames<<" init_refused="<<refused<<" factory_cases="<<factoryCases<<" raw_value_cases="<<rawCases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
#ifndef SFM64_NATIVE_CLIENT_EQUALS
static int weaponRefusal(const char*gameDirectory){
 const std::filesystem::path game=gameDirectory;auto read=[](const std::filesystem::path&p){std::ifstream in(p,std::ios::binary);gate(bool(in),"read-only VMT/GameInfo");return std::string((std::istreambuf_iterator<char>(in)),{});};auto mounts=gameinfoMountsCandidate(read(game/"usermod/gameinfo.txt"),game/"usermod",game);
 struct RecordingFactory:MaterialMultiplyPuppetProxyFactoryCandidate{std::vector<std::string>unsupported;unsigned multiplyCreated=0;MaterialProxyInstanceCandidate*create(const std::string&name)override{auto*p=MaterialMultiplyPuppetProxyFactoryCandidate::create(name);if(!p)unsupported.push_back(name);else if(name=="Multiply")++multiplyCreated;return p;}};
 MaterialObjectStoreCandidate store;auto factory=std::make_shared<RecordingFactory>();MaterialObjectParameterOptionsCandidate options;options.proxyFactory=factory;options.provider=[&](const std::string&request)->std::optional<std::string>{auto relative=sourceMaterialKeyValuesPathCandidate(request);for(const auto&mount:mounts)if(std::filesystem::is_regular_file(mount/relative))return read(mount/relative);return {};};MaterialSessionFileRuntimeCandidate runtime(store,options);
 for(const auto*name:{"models/weapons/c_items/c_flamethrower","models/weapons/c_items/c_flamethrower_blue"}){auto id=runtime.findMaterial(name);gate(id.has_value(),"actual untouched weapon VMT");gate(!runtime.initialize(*id)&&!store.get(*id)->parameterState&&!(store.get(*id)->flags&4)&&!store.get(*id)->proxyState,"full weapon stays safely uninitialized");std::cout<<"WEAPON_REFUSED\t"<<name<<'\n';}
 const std::vector<std::string>wanted={"AnimatedWeaponSheen","invis","ModelGlowColor","YellowLevel","AnimatedWeaponSheen","invis","ModelGlowColor","YellowLevel"};gate(factory->unsupported==wanted&&factory->multiplyCreated==2,"Multiply constructed on actual declarations; four remaining unsupported factory names");for(const auto&name:factory->unsupported)std::cout<<"UNSUPPORTED\t"<<name<<'\n';std::cout<<"PASS actual_weapon_materials=2 multiply_instances=2 full_weapons=REFUSED_SAFE renderer=NOT_CONNECTED\n";return 0;
}
static int multiplySafety(){
 using Store=MaterialObjectStoreCandidate;unsigned checks=0;auto check=[&](bool b){if(!b)throw std::runtime_error("Multiply safety assertion="+std::to_string(checks));++checks;};SourceMaterialPatchNodeCandidate root;root.key="UnlitTwoTexture";root.children={{"$out",3,"",0x3e800000},{"$src",3,"",0x3f400000}};SourceMaterialPatchNodeCandidate declarations;declarations.key="Proxies";SourceMaterialPatchNodeCandidate declaration;declaration.key="Multiply";declaration.children={{"resultVar",1,"$out"},{"srcVar1",1,"$src"},{"srcVar2",1,"$src"}};declarations.children.push_back(declaration);root.children.push_back(declarations);auto factory=std::make_shared<MaterialMultiplyPuppetProxyFactoryCandidate>();Store s;Store::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(root);Store::Id id=0;check(s.createMaterial("multiply/safety",source,id));MaterialObjectParameterOptionsCandidate options;options.proxyFactory=factory;check(s.initializeParameters(id,options));auto snapshot=[&](){std::ostringstream b;putValues(b,s.get(id)->parameterState->material);for(auto v:s.get(id)->forwarding.values)put(b,v);return b.str();};auto before=snapshot();auto*pointer=s.get(id)->parameterState.get();
 for(auto stage:{Store::Allocation::evaluation,Store::Allocation::evaluationCommit})check(!s.evaluateProxies(id,[&](auto kind,size_t){return kind!=stage;})&&snapshot()==before&&s.get(id)->parameterState.get()==pointer);
 unsigned failures=0;bool complete=false;for(int64_t n=0;n<4096;++n){auto balance=heapNew-heapDelete;failAfter=n;bool ok=s.evaluateProxies(id);failAfter=-1;if(ok){complete=true;break;}++failures;check(snapshot()==before&&s.get(id)->parameterState.get()==pointer&&!s.get(id)->evaluationPending);check(heapNew-heapDelete==balance);}check(complete&&failures>0);auto after=snapshot();check(after!=before);
 // A successful Multiply is usable inside the established session transaction.
 std::vector<Store::SessionEntry>rows;auto*committed=s.get(id)->parameterState.get();check(!s.withSessionDrawSteps(rows,{{id,{}}},[](const auto&)->bool{throw std::runtime_error("consumer");})&&snapshot()==after&&s.get(id)->parameterState.get()==committed);check(s.withSessionDrawSteps(rows,{{id,{}}},[](const auto&v){return v.steps.size()==1&&v.steps[0].writes->size()==1;}));
 for(const auto*bad:{"$alpha","$out[4]","$out[999999999999]"}){auto r=root;r.children.back().children.front().children.front().text=bad;Store local;Store::Source p=std::make_unique<SourceMaterialPatchNodeCandidate>(r);Store::Id x=0;check(local.createMaterial("refuse",p,x)&&!local.initializeParameters(x,options)&&!local.get(x)->parameterState);}
 {auto r=root;r.children.back().children.front().children.pop_back();Store local;Store::Source p=std::make_unique<SourceMaterialPatchNodeCandidate>(r);Store::Id x=0;check(local.createMaterial("no-second",p,x)&&!local.initializeParameters(x,options)&&!local.get(x)->parameterState);}
 auto old=MaterialPuppetFunctionProxyFactoryCandidate{};check(!old.create("Multiply"));for(auto name:{"AnimatedWeaponSheen","invis","ModelGlowColor","YellowLevel","multiply","MULTIPLY"})check(!factory->create(name));check(s.removeAndDelete(id));
 std::cout<<"PASS multiply_safety_checks="<<checks<<" actual_allocation_failures="<<failures<<" renderer=NOT_CONNECTED\n";return 0;
}
#endif
