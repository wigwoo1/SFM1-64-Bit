// ModelGlowColor reference harness reuses the verified YellowLevel test pattern.
// Production behavior is shared, not duplicated. Original prior harness is unchanged.
#ifdef SFM64_NATIVE_MODELGLOWCOLOR
#define SFM64_NATIVE_OBJECT_PARAMETERS
#endif
#define SFM64_OBJECT_PARAMETER_MAIN glowPriorParameterMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#ifdef SFM64_NATIVE_MODELGLOWCOLOR
#include "material_equals_client_native_fixture.h"
static void __fastcall yellowVector3(void*p,void*,uint32_t a,uint32_t b,uint32_t c){auto&f=MaterialEqualsClientNativeFixture::self();auto i=f.index(p);gate(materialTypedNumericWriteCandidate(f.variables[i].value,3,{a,b,c,0}),"checked native vector3 value service");f.writes.push_back({i,3,0,{a,b,c,0}});f.refresh(i);}
#else
#include "material_modelglowcolor_proxy_candidate.h"
#include "material_session_file_runtime_candidate.h"
#include "gameinfo_mounts_candidate.h"
static int safety();static int weaponRefusal(const char*directory);
#endif
static void putWrites(std::ostream&out,const std::vector<MaterialProxyWriteCandidate>&writes){put(out,uint32_t(writes.size()));for(const auto&w:writes){put(out,w.index);put(out,w.operation);put(out,w.count);for(auto b:w.arguments)put(out,b);}}
static MaterialObjectParameterPlanCandidate readPrepared(std::istream&in,const SourceMaterialPatchNodeCandidate&root){MaterialObjectParameterPlanCandidate p;p.finalRoot=root;p.state=std::make_unique<MaterialObjectParameterStateCandidate>();auto&m=p.state->material;m.shader=text(in);auto count=word(in);gate(count>=13&&count<=255,"prepared variable count");for(unsigned i=0;i<count;++i){std::array<uint32_t,28>q{};for(auto&n:q)n=word(in);MaterialTypedDispatchCandidate v;v.created=q[0]!=0;v.value.numeric.typeAndMetadata=q[1];v.value.numeric.integer=q[2];std::copy_n(q.begin()+3,4,v.value.numeric.vector.begin());gate(q[10]<4096&&q[11]<1024,"prepared text extent");v.value.text.resize(q[10]);v.name.resize(q[11]);in.read(v.value.text.data(),q[10]);in.read(v.name.data(),q[11]);if(q[8]){v.matrix.emplace();std::copy_n(q.begin()+12,16,v.matrix->begin());}v.matrixIdentity=q[9]!=0;m.parameters.values.push_back(std::move(v));}gate(bool(in),"complete prepared state");return p;}
static const SourceMaterialPatchNodeCandidate&declarations(const MaterialObjectParameterPlanCandidate&plan){auto*p=materialProxySelectedNodeCandidate(plan.finalRoot,plan.state->material.selectedNode);gate(p!=nullptr,"selected node");auto*r=sourceMaterialPatchFindConstCandidate(*p,"Proxies");if(!r)r=sourceMaterialPatchFindConstCandidate(plan.finalRoot,"Proxies");gate(r!=nullptr,"declarations required");return *r;}
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_MODELGLOWCOLOR
 if(argc==1)return safety();if(argc==3&&std::string(argv[1])=="--weapon-refusal")return weaponRefusal(argv[2]);static_assert(sizeof(void*)==8,"owned x64 YellowLevel");
#endif
 gate(argc==5&&!std::ifstream(argv[4]).good(),"client-or-absent query original-prepared fresh-output");std::ifstream in(argv[2],std::ios::binary),prepared(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);gate(bool(in)&&bool(prepared)&&bool(out),"ModelGlow IO");
#ifdef SFM64_NATIVE_MODELGLOWCOLOR
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);MaterialEqualsClientNativeFixture client(argv[1]);client.varMethods[10]=reinterpret_cast<uintptr_t>(&yellowVector3);client.word(0x63ce48,0); // Deliberately unavailable clock; no original import/engine supplied.
#endif
 auto count=word(in);gate(count&&count<10000&&word(prepared)==count,"case count");put(out,count);unsigned frames=0,refused=0;
 for(unsigned ci=0;ci<count;++ci){auto bind=word(in);auto root=node(in);auto plan=readPrepared(prepared,root);const auto&decls=declarations(plan);gate(!decls.children.empty(),"nonempty declaration list");
#ifdef SFM64_NATIVE_MODELGLOWCOLOR
  client.prepare(*plan.state);std::vector<std::pair<void*,uintptr_t>>proxies;bool initialized=true;
  for(const auto&d:decls.children){uintptr_t factory=0,table=0,init=0,bindMethod=0;
   if(d.key=="ModelGlowColor"){factory=0x10225b20;table=0x10535524;init=0x100e5db0;bindMethod=0x102247b0;}
   else if(d.key=="YellowLevel"){factory=0x10225af0;table=0x1053550c;init=0x100e5db0;bindMethod=0x102247b0;}
   else if(d.key=="Multiply"){factory=0x10137010;table=0x104ffeac;init=0x10136c20;bindMethod=0x10136f00;}
   else if(d.key=="Equals"){factory=0x10137430;table=0x104fff3c;init=0x100e5fa0;bindMethod=0x101373a0;}
   else throw std::runtime_error("unmeasured reference name");using Create=void*(__cdecl*)();auto*p=reinterpret_cast<Create>(factory)();gate(p&&*static_cast<uintptr_t*>(p)==table,"registered factory/vtable");using Init=bool(__thiscall*)(void*,void*,void*);if(!reinterpret_cast<Init>(init)(p,&client.material,client.build(d))){client.destroy(p);initialized=false;break;}gate(client.materialIdentity(p),"material identity");proxies.push_back({p,bindMethod});}
#else
  MaterialObjectStoreCandidate store;uintptr_t id=0;MaterialObjectParameterOptionsCandidate options;options.proxyFactory=std::make_shared<MaterialModelGlowColorProxyFactoryCandidate>();MaterialObjectParameterPlanCandidate own;if(!materialObjectParameterPrepareCandidate(own,"modelglow/case",0x10,&root,options))throw std::runtime_error("owned CPU prepare case="+std::to_string(ci));std::ostringstream actual,expected;putValues(actual,own.state->material);putValues(expected,plan.state->material);gate(actual.str()==expected.str(),"owned initial state matches separately executed original CPU state");MaterialObjectStoreCandidate::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(root);gate(store.createMaterial("modelglow/case",source,id),"owned material");const bool initialized=store.initializeParameters(id,options);
#endif
  put(out,ci);put(out,initialized);if(!initialized)++refused;
  if(initialized)for(unsigned frame=0;frame<3;++frame){
#ifdef SFM64_NATIVE_MODELGLOWCOLOR
   client.writes.clear();using Bind=void(__thiscall*)(void*,void*);for(auto p:proxies)reinterpret_cast<Bind>(p.second)(p.first,reinterpret_cast<void*>(uintptr_t(bind)));client.image.verify();putWrites(out,client.writes);auto state=*plan.state;state.material.parameters.values.clear();for(const auto&v:client.variables)state.material.parameters.values.push_back(v.value);
#else
   MaterialProxyEvaluationInputsCandidate inputs;if(bind)inputs.currentTimeBits=bind; // Unknown model and absent/varied clock must not affect this proxy.
   MaterialProxyEvaluationAttemptCandidate attempt;gate(store.evaluateProxies(id,{},&attempt,inputs)&&attempt.evaluated,"actual owned proxy frame");putWrites(out,attempt.writes);const auto&state=*store.get(id)->parameterState;
#endif
   putValues(out,state.material);std::array<uint32_t,7>forward{};gate(materialObjectParameterForwardCandidate(forward,state),"forward values");for(auto n:forward)put(out,n);++frames;
  }
#ifdef SFM64_NATIVE_MODELGLOWCOLOR
  for(auto i=proxies.rbegin();i!=proxies.rend();++i)client.destroy(i->first);gate(client.blocks.empty(),"native case cleanup");client.image.verify();
#else
  gate(store.removeAndDelete(id),"owned case cleanup");
#endif
 }
 gate(in.peek()==std::char_traits<char>::eof()&&prepared.peek()==std::char_traits<char>::eof(),"complete input tail");out.flush();gate(bool(out),"complete output");
#ifdef SFM64_NATIVE_MODELGLOWCOLOR
 std::cout<<"native_proxy_allocations="<<client.allocations<<" releases="<<client.releases<<" clock_pointer=NULL\n";
#endif
 std::cout<<"PASS modelglowcolor_cases="<<count<<" frames="<<frames<<" init_refused="<<refused<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
#ifndef SFM64_NATIVE_MODELGLOWCOLOR
static SourceMaterialPatchNodeCandidate seed(){SourceMaterialPatchNodeCandidate root;root.key="UnlitTwoTexture";root.children={{"$yellow",3,"",0x3e800000}};SourceMaterialPatchNodeCandidate block;block.key="Proxies";SourceMaterialPatchNodeCandidate proxy;proxy.key="ModelGlowColor";proxy.children={{"resultVar",1,"$yellow[999]"}};block.children.push_back(proxy);root.children.push_back(block);return root;}
static int safety(){
 using Store=MaterialObjectStoreCandidate;unsigned checks=0;auto check=[&](bool b){if(!b)throw std::runtime_error("Yellow safety="+std::to_string(checks));++checks;};auto factory=std::make_shared<MaterialModelGlowColorProxyFactoryCandidate>();Store store;auto root=seed();Store::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(root);auto*sourcePointer=source.get();Store::Id id=0;check(store.createMaterial("yellow/safety",source,id));MaterialObjectParameterOptionsCandidate options;options.proxyFactory=factory;check(store.initializeParameters(id,options));auto snapshot=[&](){std::ostringstream s;putValues(s,store.get(id)->parameterState->material);for(auto n:store.get(id)->forwarding.values)put(s,n);return s.str();};auto before=snapshot();auto*pointer=store.get(id)->parameterState.get();
 for(auto stage:{Store::Allocation::evaluation,Store::Allocation::evaluationCommit})check(!store.evaluateProxies(id,[&](auto kind,size_t){return kind!=stage;})&&snapshot()==before&&store.get(id)->parameterState.get()==pointer);
 check(!store.evaluateProxies(id,[&](auto kind,size_t){if(kind==Store::Allocation::evaluationCommit)sourcePointer->children[0].bits=0;return true;})&&snapshot()==before&&store.get(id)->parameterState.get()==pointer);*sourcePointer=root;
 unsigned failures=0;bool complete=false;for(int64_t n=0;n<4096;++n){auto balance=heapNew-heapDelete;failAfter=n;bool ok=store.evaluateProxies(id);failAfter=-1;if(ok){complete=true;break;}++failures;check(snapshot()==before&&store.get(id)->parameterState.get()==pointer&&!store.get(id)->evaluationPending);check(heapNew-heapDelete==balance);}check(complete&&failures>0);
 auto value=[&]()->const MaterialTypedDispatchCandidate&{const auto&v=store.get(id)->parameterState->material.parameters.values;auto i=std::find_if(v.begin(),v.end(),[](const auto&x){return x.name=="$yellow";});gate(i!=v.end(),"yellow result exists");return *i;};check((value().value.numeric.typeAndMetadata&0x7f)==0x32&&value().value.numeric.vector==std::array<uint32_t,4>{0x3f800000,0x3f800000,0x3f800000,0});auto after=snapshot();pointer=store.get(id)->parameterState.get();for(unsigned mode=0;mode<2;++mode)check(!store.withSessionDrawSteps({},{{id,{}}},[&](const auto&)->bool{if(mode)throw std::runtime_error("consumer");return false;})&&snapshot()==after&&store.get(id)->parameterState.get()==pointer);
 MaterialSessionOverrideInputCandidate row;row.name="$yellow";row.typeFlags=3;row.words[0]=0x3f000000;check(store.withSessionDrawSteps({{id,{row}}},{{id,{}}},[](const auto&view){return view.steps.size()==1&&view.steps[0].writes->size()==1&&view.steps[0].writes->at(0).operation==3;}));check((value().value.numeric.typeAndMetadata&15u)==0&&value().value.numeric.vector[0]==0x3f800000); // Native float backup projects prior vector component0.
 for(const auto*output:{"$missing","$alpha"}){Store other;auto r=seed();r.children.back().children.front().children.front().text=output;Store::Source p=std::make_unique<SourceMaterialPatchNodeCandidate>(r);Store::Id x=0;check(other.createMaterial("refused",p,x)&&!other.initializeParameters(x,options)&&!other.get(x)->parameterState);}
 {Store other;auto r=seed();r.children.back().children.front().children.front().text=std::string("$yellow\0ignored",15);Store::Source p=std::make_unique<SourceMaterialPatchNodeCandidate>(r);Store::Id x=0;check(other.createMaterial("embedded-null",p,x)&&!other.initializeParameters(x,options)&&!other.get(x)->parameterState);}
 {MaterialYellowLevelProxyFactoryCandidate old;check(!old.create("ModelGlowColor"));for(auto name:{"AnimatedWeaponSheen","invis","modelglowcolor","MODELGLOWCOLOR"})check(!factory->create(name));}
 check(store.uncacheParameters(id)&&store.initializeParameters(id,options)&&store.evaluateProxies(id)&&store.removeAndDelete(id));std::cout<<"PASS modelglowcolor_safety_checks="<<checks<<" actual_allocation_failures="<<failures<<" clock=NOT_REQUIRED context=NOT_REQUIRED renderer=NOT_CONNECTED\n";return 0;
}
static int weaponRefusal(const char*directory){
 const std::filesystem::path game=directory;auto read=[](const std::filesystem::path&p){std::ifstream in(p,std::ios::binary);gate(bool(in),"read-only original");return std::string((std::istreambuf_iterator<char>(in)),{});};auto mounts=gameinfoMountsCandidate(read(game/"usermod/gameinfo.txt"),game/"usermod",game);
 struct Recording:MaterialModelGlowColorProxyFactoryCandidate{std::vector<std::string>unsupported;unsigned yellow=0,multiply=0,glow=0;MaterialProxyInstanceCandidate*create(const std::string&name)override{auto*p=MaterialModelGlowColorProxyFactoryCandidate::create(name);if(!p)unsupported.push_back(name);else{yellow+=name=="YellowLevel";multiply+=name=="Multiply";glow+=name=="ModelGlowColor";}return p;}};
 auto factory=std::make_shared<Recording>();MaterialObjectStoreCandidate store;MaterialObjectParameterOptionsCandidate options;options.proxyFactory=factory;options.provider=[&](const std::string&request)->std::optional<std::string>{auto relative=sourceMaterialKeyValuesPathCandidate(request);for(const auto&mount:mounts)if(std::filesystem::is_regular_file(mount/relative))return read(mount/relative);return {};};MaterialSessionFileRuntimeCandidate runtime(store,options);
 for(const auto*name:{"models/weapons/c_items/c_flamethrower","models/weapons/c_items/c_flamethrower_blue"}){auto id=runtime.findMaterial(name);gate(id&& !runtime.initialize(*id)&&!store.get(*id)->parameterState&&!store.get(*id)->proxyState&&!(store.get(*id)->flags&4),"actual complete weapon remains unavailable");std::cout<<"WEAPON_REFUSED\t"<<name<<'\n';}
 gate(factory->yellow==2&&factory->multiply==2&&factory->glow==2&&factory->unsupported==std::vector<std::string>{"AnimatedWeaponSheen","invis","AnimatedWeaponSheen","invis"},"actual supported constructors and remaining two factory gaps");for(const auto&name:factory->unsupported)std::cout<<"UNSUPPORTED\t"<<name<<'\n';std::cout<<"PASS actual_weapon_materials=2 glow_instances=2 yellow_instances=2 multiply_instances=2 complete_materials=REFUSED_SAFE GPU=NOT_RUN\n";return 0;
}
#endif
