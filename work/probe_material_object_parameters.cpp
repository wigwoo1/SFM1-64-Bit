#include <cstdlib>
#include <new>
#include <cstdint>
static void*observedSource=nullptr;static bool observedSourceFreed=false;static int64_t failAfter=-1;static size_t heapNew=0,heapDelete=0;
static void noteFree(void*p){if(p&&p==observedSource)observedSourceFreed=true;}
void*operator new(size_t n){if(failAfter>=0&&failAfter--==0)throw std::bad_alloc();auto*p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();++heapNew;return p;}
void operator delete(void*p)noexcept{if(p)++heapDelete;noteFree(p);std::free(p);}
void operator delete(void*p,size_t)noexcept{::operator delete(p);}
void*operator new[](size_t n){return ::operator new(n);}
void operator delete[](void*p)noexcept{::operator delete(p);}
void operator delete[](void*p,size_t)noexcept{::operator delete(p);}
#include "material_object_parameter_initialization_candidate.h"
#ifdef SFM64_NATIVE_OBJECT_PARAMETERS
#include "material_object_parameters_native_fixture.h"
#else
#include "material_object_store_candidate.h"
#include "source_material_source_provider_candidate.h"
#endif
#include <fstream>
#include <iostream>
#include <sstream>
static void gate(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t word(std::istream&in){uint32_t v;gate(bool(in.read(reinterpret_cast<char*>(&v),4)),"Word input");return v;}
static std::string text(std::istream&in){auto n=word(in);gate(n<=4095,"Text extent");std::string s(n,'\0');gate(bool(in.read(s.data(),n)),"Text input");return s;}
static SourceMaterialPatchNodeCandidate node(std::istream&in,unsigned depth=0){gate(depth<16,"Input tree depth");SourceMaterialPatchNodeCandidate n;n.kind=word(in);n.bits=word(in);n.key=text(in);n.text=text(in);auto count=word(in);gate(count<256,"Input children");for(unsigned i=0;i<count;++i)n.children.push_back(node(in,depth+1));return n;}
static void put(std::ostream&out,uint32_t n){out.write(reinterpret_cast<const char*>(&n),4);}
static void putText(std::ostream&out,const std::string&s){put(out,uint32_t(s.size()));out.write(s.data(),s.size());}
static void putTree(std::ostream&out,const SourceMaterialPatchNodeCandidate&n){putText(out,n.key);put(out,n.kind);put(out,n.bits);putText(out,n.text);out.write(reinterpret_cast<const char*>(&n.pointer),8);put(out,uint32_t(n.children.size()));for(const auto&c:n.children)putTree(out,c);}
static void putValues(std::ostream&out,const MaterialParameterOrchestrationCandidate&r){
    putText(out,r.shader);put(out,uint32_t(r.parameters.values.size()));
    for(const auto&v:r.parameters.values){std::array<uint32_t,28>q{};q[0]=v.created?1u:0u;q[1]=v.value.numeric.typeAndMetadata;q[2]=v.value.numeric.integer;std::copy(v.value.numeric.vector.begin(),v.value.numeric.vector.end(),q.begin()+3);q[7]=(q[1]&15)==5?1u:(q[1]&15)==8?2u:0u;q[8]=v.matrix?1u:0u;q[9]=v.matrixIdentity?1u:0u;q[10]=uint32_t(v.value.text.size());q[11]=uint32_t(v.name.size());if(v.matrix)std::copy(v.matrix->begin(),v.matrix->end(),q.begin()+12);out.write(reinterpret_cast<const char*>(q.data()),sizeof(q));out.write(v.value.text.data(),v.value.text.size());out.write(v.name.data(),v.name.size());}
}
struct CaseInput{
    unsigned route=0,repeats=0,uncache=0,queued=0;std::string name;SourceMaterialPatchNodeCandidate root;
    MaterialObjectParameterOptionsCandidate options;
};
#ifdef SFM64_NATIVE_OBJECT_PARAMETERS
struct ObjectParameterFixture:MaterialObjectParametersNativeFixture {
    using MaterialObjectParametersNativeFixture::MaterialObjectParametersNativeFixture;
    static void __fastcall trackedHeapFree(void*a,void*b,void*p){noteFree(p);MaterialFallbackNativeFixture::heapFree(a,b,p);}
    static void __fastcall trackedKVFree(void*a,void*b,void*p){noteFree(p);MaterialFallbackNativeFixture::kvFree(a,b,p);}
    void beginCase(const CaseInput&c){beginParameters();allocators[5]=reinterpret_cast<uintptr_t>(&trackedHeapFree);kvMethods[2]=reinterpret_cast<uintptr_t>(&trackedKVFree);configure(c.options.environment,c.options.settings,c.options.hardwareLevel);}
};
struct ObjectParameterAdapter {
    ObjectParameterFixture&f;uint8_t*raw=nullptr,*error=nullptr;void*external=nullptr,*patch=nullptr;bool alive=true,synced=false;unsigned route=0;
    ObjectParameterAdapter(ObjectParameterFixture&fixture,const CaseInput&c):f(fixture),route(c.route){
        f.beginCase(c);auto seed=builtinErrorMaterialSeedCandidate();error=f.createActual(seed.name.c_str(),f.build(seed.source),false);f.word(0x1059ec,reinterpret_cast<uintptr_t>(error));
        observedSource=c.route==0?f.build(c.root):nullptr;observedSourceFreed=false;raw=f.createActual(c.name.c_str(),observedSource);
        SourceMaterialPatchNodeCandidate ignored;ignored.key="Wireframe_DX9";external=f.build(c.route==1?c.root:ignored);
        SourceMaterialPatchNodeCandidate accumulator;accumulator.key="patchAccumulator";accumulator.children.push_back({"sentinel",1,"preserve-until-a-file-load"});patch=f.build(accumulator);
    }
    bool initialize(){bool ok=f.initializeActual(raw,route==2?nullptr:external,patch);if(ok&&raw[0x22])synced=true;return ok;}
    void uncache(bool preserve){f.uncacheActual(raw,preserve);}
    void mutate(){auto*root=route==0?observedSource:external;if(route==2)return;using F=void(__thiscall*)(void*,const char*,const char*);reinterpret_cast<F>(0x10070db0)(root,"$alpha",".125");}
    void enqueueDelete(bool queued){f.enabled=queued;f.forward(f.id(raw),13);f.forward(f.id(raw),51);if(!queued)alive=false;}
    void drain(){f.drain();alive=false;}
    void record(std::ostream&out,unsigned index,unsigned stage,bool result){
        put(out,index);put(out,stage);put(out,result);put(out,alive);put(out,observedSource?1u:0u);put(out,observedSourceFreed);put(out,alive?f.read(raw,0x1c):0);put(out,alive?f.read(raw,0x20)&0xffff:0);put(out,alive?raw[0x22]:0);
        put(out,alive&&synced);for(size_t i=0;i<7;++i)put(out,alive&&synced?f.read(raw,0x88+i*4):0);
        put(out,f.warnings+f.shaderWarnings);put(out,f.developerMessages);put(out,unsigned(f.lookups.size()));for(const auto&s:f.lookups)putText(out,s);put(out,unsigned(f.queued().size()));
        put(out,alive&&observedSource);if(alive&&observedSource){gate(f.read(raw,0x7c)==reinterpret_cast<uintptr_t>(observedSource),"Root identity preserved");putTree(out,f.extract(observedSource));}
        putTree(out,f.extract(external));putTree(out,f.extract(patch));
        MaterialParameterOrchestrationCandidate value;
        if(alive&&raw[0x22]){
            const auto shader=f.read(raw,0x14),array=f.read(raw,0x24);for(size_t i=0;i<f.shaders.size();++i)if(reinterpret_cast<uintptr_t>(f.shaders[i])==shader)value.shader=f.shaderNames[i];gate(!value.shader.empty()&&array,"Actual shader/array publication");
            for(unsigned i=0;i<raw[0x22];++i){auto*b=static_cast<uint8_t*>(reinterpret_cast<void**>(array)[i]);gate(f.read(b,32)==reinterpret_cast<uintptr_t>(raw),"Actual parameter owner is normally constructed material");MaterialTypedDispatchCandidate v;v.created=true;v.value.numeric.typeAndMetadata=f.read(b,28)&0xffff;v.value.numeric.integer=f.read(b,8);std::memcpy(v.value.numeric.vector.data(),b+12,16);using Name=const char*(__thiscall*)(void*);v.name=reinterpret_cast<Name>(0x10017b80)(b);auto type=v.value.numeric.typeAndMetadata&15;if(type==1)v.value.text=reinterpret_cast<const char*>(f.read(b,4));if(type==7){auto*p=reinterpret_cast<const uint8_t*>(f.read(b,36));gate(p!=nullptr,"Actual matrix payload");v.matrix.emplace();std::memcpy(v.matrix->data(),p,64);v.matrixIdentity=p[64]!=0;}value.parameters.values.push_back(std::move(v));}
        }
        putValues(out,value);gate(f.tokens.empty(),"Nested native tokens balanced");f.verifyCode();f.cacheGuards();
    }
    void finish(){gate(!alive&&(!observedSource||observedSourceFreed),"Native object/source deletion");f.destroyTree(external);f.destroyTree(patch);f.destroyActual(error);f.stop();}
};
#else
struct ObjectParameterAdapter {
    MaterialObjectStoreCandidate store;uintptr_t id=0;SourceMaterialPatchNodeCandidate external,patch;CaseInput input;bool alive=true;uint32_t warnings=0,developer=0;std::vector<std::string>lookups;
    ObjectParameterAdapter(const CaseInput&c,const SourceMaterialKeyValuesSourceCandidate&provider):input(c){
        MaterialObjectStoreCandidate::Source source;if(c.route==0)source=std::make_unique<SourceMaterialPatchNodeCandidate>(c.root);observedSource=source.get();observedSourceFreed=false;gate(store.createMaterial(c.name,source,id),"Owned material construction");external=c.route==1?c.root:SourceMaterialPatchNodeCandidate{"Wireframe_DX9"};input.options.provider=provider;
        patch.key="patchAccumulator";patch.children.push_back({"sentinel",1,"preserve-until-a-file-load"});
    }
    bool initialize(){auto options=input.options;options.externalSource=input.route==2?nullptr:&external;options.externalPatch=&patch;MaterialObjectParameterAttemptCandidate attempt;bool ok=store.initializeParameters(id,options,{},&attempt);warnings+=attempt.warnings;developer+=attempt.developerDiagnostics;if(attempt.status==MaterialObjectParameterStatusCandidate::Initialized){const auto&m=store.get(id)->parameterState->material;lookups.insert(lookups.end(),m.lookups.begin(),m.lookups.end());}return ok;}
    void uncache(bool preserve){gate(store.uncacheParameters(id,preserve),"Owned uncache");}
    void mutate(){if(input.route==2)return;auto*root=input.route==0?static_cast<SourceMaterialPatchNodeCandidate*>(observedSource):&external;auto*p=sourceMaterialPatchFindCandidate(*root,"$alpha",true);p->kind=1;p->text=".125";p->bits=0;p->pointer=0;}
    void enqueueDelete(bool queued){gate(store.forward(id,MaterialObjectStoreCandidate::Operation::decrement,queued)&&store.forward(id,MaterialObjectStoreCandidate::Operation::deleteIfUnreferenced,queued),"Owned deferred teardown");if(!queued)alive=false;}
    void drain(){store.drain();alive=false;}
    void record(std::ostream&out,unsigned index,unsigned stage,bool result){
        const auto*p=store.get(id);put(out,index);put(out,stage);put(out,result);put(out,alive);put(out,observedSource?1u:0u);put(out,observedSourceFreed);put(out,p?uint32_t(p->references.load()):0);put(out,p?p->flags:0);put(out,p?uint32_t(p->parameterCount):0);put(out,p&&p->forwarding.valuesInitialized);for(size_t i=0;i<7;++i)put(out,p&&p->forwarding.valuesInitialized?p->forwarding.values[i]:0);
        put(out,warnings);put(out,developer);put(out,unsigned(lookups.size()));for(const auto&s:lookups)putText(out,s);put(out,unsigned(store.queuedCount()));put(out,p&&observedSource);if(p&&observedSource){gate(p->source.get()==observedSource,"Owned root identity preserved");putTree(out,*p->source);}putTree(out,external);putTree(out,patch);putValues(out,p&&p->parameterState?p->parameterState->material:MaterialParameterOrchestrationCandidate{});gate(store.valid(),"Owned dictionary structure");
    }
    void finish(){gate(!alive&&(!observedSource||observedSourceFreed)&&store.size()==0&&store.cached().empty(),"Owned complete object/parameter/source teardown");}
};
static int parameterSafety();
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_OBJECT_PARAMETERS
    if(argc==2&&std::string(argv[1])=="--test")return parameterSafety();static_assert(sizeof(void*)==8,"Owned initialization is x64");
#endif
    gate(argc==6,"Material-or-absent library-or-absent disposable mount input fresh output");gate(!std::ifstream(argv[5]).good(),"Fresh output required");std::ifstream in(argv[4],std::ios::binary);std::ofstream out(argv[5],std::ios::binary);gate(bool(in)&&bool(out),"Case IO");
#ifdef SFM64_NATIVE_OBJECT_PARAMETERS
    ObjectParameterFixture fixture(argv[1],argv[2],argv[3]);auto observer=AddVectoredExceptionHandler(1,&lifetimeFault);unsigned oldControl=0,control=0;_controlfp_s(&oldControl,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    SourceMaterialSourceProviderCandidate files({{std::filesystem::path(argv[3]),"GAME"}});auto provider=files.materialSource();
#endif
    auto oldMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);auto count=word(in);gate(count&&count<10000,"Case count");put(out,count);unsigned stages=0;
    for(unsigned index=0;index<count;++index){CaseInput c;c.route=word(in);c.repeats=word(in);c.uncache=word(in);c.queued=word(in);gate(c.route<=2&&c.repeats<=1&&c.uncache<=1&&c.queued<=1,"Case booleans");std::array<uint32_t,12>q{};gate(bool(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))),"Case environment");c.name=text(in);c.root=node(in);auto&env=c.options.environment;auto&s=c.options.settings;std::memcpy(&env.condition.gpu,&q[0],4);std::memcpy(&env.condition.hdr,&q[1],4);env.condition.srgbReturn=q[2];std::memcpy(&env.condition.lowFill,&q[3],4);env.percentGateReturn=q[4];env.percentKeepReturn=q[5];std::memcpy(&env.percentOverride,&q[6],4);s.configFlagsAt34=q[7];std::memcpy(&s.matPhong,&q[8],4);std::memcpy(&s.forceVertexFog,&q[9],4);std::memcpy(&c.options.hardwareLevel,&q[10],4);s.toolsSwitchPresent=q[11]!=0;
#ifdef SFM64_NATIVE_OBJECT_PARAMETERS
        currentCase=index;ObjectParameterAdapter a(fixture,c);
#else
        ObjectParameterAdapter a(c,provider);
#endif
        a.record(out,index,0,false);++stages;bool result=a.initialize();a.record(out,index,1,result);++stages;
        if(result&&c.repeats){a.mutate();result=a.initialize();a.record(out,index,2,result);++stages;}
        if(result&&c.uncache){a.uncache(true);a.record(out,index,3,true);++stages;a.uncache(false);a.record(out,index,4,true);++stages;result=a.initialize();a.record(out,index,5,result);++stages;}
        a.enqueueDelete(c.queued!=0);a.record(out,index,6,true);++stages;if(c.queued){a.drain();a.record(out,index,7,true);++stages;}a.finish();
    }
    gate(in.peek()==std::char_traits<char>::eof(),"Query tail");out.flush();gate(bool(out),"Complete output");_mm_setcsr(oldMxcsr);
#ifdef SFM64_NATIVE_OBJECT_PARAMETERS
    fixture.finishLifetime();RemoveVectoredExceptionHandler(observer);_controlfp_s(&control,oldControl,_MCW_RC|_MCW_PC);std::cout<<"native_allocations="<<fixture.allocations<<" releases="<<fixture.releases<<'\n';
#endif
    std::cout<<"PASS object_parameter_cases="<<count<<" stages="<<stages<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
#ifndef SFM64_NATIVE_OBJECT_PARAMETERS
static int parameterSafety(){
    static_assert(std::is_const<std::remove_reference_t<decltype(*std::declval<const MaterialObjectStoreCandidate::Object&>().parameterState)>>::value,"Published parameter state must stay read-only");
    using Store=MaterialObjectStoreCandidate;unsigned checks=0;auto check=[&](bool b){gate(b,"Owned object parameter safety");++checks;};
    Store store;Store::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>();source->key="UnlitGeneric";source->children.push_back({"$alpha",1,".5"});auto*root=source.get();auto*child=&source->children.front();uintptr_t id=0;check(store.createMaterial("safety/material",source,id));auto snapshot=[&](){std::ostringstream out;auto*p=store.get(id);put(out,p->flags);put(out,uint32_t(p->parameterCount));putTree(out,*p->source);if(p->parameterState)putValues(out,p->parameterState->material);return out.str();};const auto before=snapshot();
    for(auto stage:{Store::Allocation::parameters,Store::Allocation::parameterCommit}){const auto balance=heapNew-heapDelete;check(!store.initializeParameters(id,{},[&](Store::Allocation k,size_t){return k!=stage;})&&snapshot()==before&&store.get(id)->source.get()==root&&&root->children.front()==child);check(heapNew-heapDelete==balance);}
    unsigned failures=0;bool succeeded=false;
    for(int64_t n=0;n<2048;++n){auto balance=heapNew-heapDelete;failAfter=n;bool ok=store.initializeParameters(id);failAfter=-1;if(ok){succeeded=true;break;}++failures;bool unchanged=snapshot()==before;check(unchanged&&heapNew-heapDelete==balance&&store.get(id)->source.get()==root&&&root->children.front()==child);}
    check(succeeded&&failures>50&&store.get(id)->parameterCount==68&&store.get(id)->parameterState&&store.get(id)->source.get()==root&&&root->children.front()==child);const auto initialized=snapshot();
    MaterialObjectParameterOptionsCandidate ignored;ignored.provider=[](const std::string&)->std::optional<std::string>{throw std::runtime_error("Must not read on initialized guard");};check(store.initializeParameters(id,ignored)&&snapshot()==initialized);check(store.uncacheParameters(id,true)&&snapshot()==initialized);check(store.uncacheParameters(id)&&!store.get(id)->parameterState&&store.get(id)->source.get()==root);
    root->key="LightmappedGeneric";const auto unsupported=snapshot();check(!store.initializeParameters(id)&&snapshot()==unsupported);root->key="UnlitGeneric";SourceMaterialPatchNodeCandidate proxies;proxies.key="Proxies";proxies.children.push_back({"AnimatedTexture",0});root->children.push_back(proxies);const auto withProxy=snapshot();check(!store.initializeParameters(id)&&snapshot()==withProxy);root->children.pop_back();
    check(store.initializeParameters(id));check(store.forward(id,Store::Operation::decrement,true)&&store.forward(id,Store::Operation::deleteIfUnreferenced,true)&&store.get(id)->parameterState);store.drain();check(!store.get(id)&&store.size()==0&&!store.initializeParameters(id)&&!store.uncacheParameters(id));
    auto fallback=[](){auto n=std::make_unique<SourceMaterialPatchNodeCandidate>();n->key="Wireframe";n->children.push_back({"$fallbackmaterial",1,"target"});return n;};
    {Store callbacks;source=fallback();check(callbacks.createMaterial("callbacks/nested",source,id));bool nestedRefused=false;MaterialObjectParameterOptionsCandidate options;options.provider=[&](const std::string&)->std::optional<std::string>{nestedRefused=!callbacks.initializeParameters(id);return "UnlitGeneric{}";};check(callbacks.initializeParameters(id,options)&&nestedRefused&&callbacks.get(id)->parameterCount==68);callbacks.decrement(id);check(callbacks.deleteIfUnreferenced(id));}
    {Store callbacks;source=fallback();check(callbacks.createMaterial("callbacks/deleted",source,id));MaterialObjectParameterOptionsCandidate options;options.provider=[&](const std::string&)->std::optional<std::string>{callbacks.removeAndDelete(id);return "UnlitGeneric{}";};check(!callbacks.initializeParameters(id,options)&&callbacks.size()==0);}
    {Store callbacks;source=fallback();check(callbacks.createMaterial("callbacks/commit",source,id));check(!callbacks.initializeParameters(id,{},[&](Store::Allocation stage,size_t){if(stage==Store::Allocation::parameters)callbacks.removeAndDelete(id);return true;})&&callbacks.size()==0);}
    {Store borrowed;Store::Source empty;check(borrowed.createMaterial("callbacks/borrowed",empty,id));auto external=*fallback();SourceMaterialPatchNodeCandidate aggregate;aggregate.key="patchAccumulator";aggregate.children.push_back({"sentinel",1,"keep"});const auto originalAggregate=aggregate;MaterialObjectParameterOptionsCandidate options;options.externalSource=&external;options.externalPatch=&aggregate;options.provider=[&](const std::string&)->std::optional<std::string>{sourceMaterialPatchFindCandidate(external,"$alpha",true)->text="external edit";return "UnlitGeneric{}";};check(!borrowed.initializeParameters(id,options)&&!borrowed.get(id)->parameterState&&materialObjectParameterTreeEqualCandidate(aggregate,originalAggregate));borrowed.decrement(id);check(borrowed.deleteIfUnreferenced(id));}
    {Store refused;source=fallback();check(refused.createMaterial("safety/fallback-proxy",source,id));MaterialObjectParameterOptionsCandidate options;options.provider=[](const std::string&)->std::optional<std::string>{return "UnlitGeneric{Proxies{AnimatedTexture{}}}";};check(!refused.initializeParameters(id,options)&&!refused.get(id)->parameterState&&refused.get(id)->flags==16);refused.decrement(id);check(refused.deleteIfUnreferenced(id));}
    std::cout<<"PASS object_parameter_safety_checks="<<checks<<" actual_allocation_failures="<<failures<<'\n';return 0;
}
#endif
