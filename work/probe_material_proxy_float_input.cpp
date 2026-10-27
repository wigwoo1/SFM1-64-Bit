#ifdef SFM64_NATIVE_CLIENT_EQUALS
#define SFM64_NATIVE_OBJECT_PARAMETERS
#endif
#define SFM64_OBJECT_PARAMETER_MAIN floatInputPriorParameterMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#include "material_proxy_float_input_candidate.h"
static const SourceMaterialPatchNodeCandidate*equalsDeclarations(const MaterialObjectParameterPlanCandidate&plan){auto*p=materialProxySelectedNodeCandidate(plan.finalRoot,plan.state->material.selectedNode);gate(p!=nullptr,"Selected node");auto*r=sourceMaterialPatchFindConstCandidate(*p,"Proxies");return r?r:sourceMaterialPatchFindConstCandidate(plan.finalRoot,"Proxies");}
static void putWrites(std::ostream&out,const std::vector<MaterialProxyWriteCandidate>&writes){put(out,uint32_t(writes.size()));for(const auto&w:writes){put(out,w.index);put(out,w.operation);put(out,w.count);for(auto b:w.arguments)put(out,b);}}
static MaterialObjectParameterPlanCandidate readPrepared(std::istream&in,const SourceMaterialPatchNodeCandidate&root){
    MaterialObjectParameterPlanCandidate p;p.finalRoot=root;p.state=std::make_unique<MaterialObjectParameterStateCandidate>();auto&m=p.state->material;m.shader=text(in);auto count=word(in);gate(count>=13&&count<=255,"Prepared original variable count");
    for(unsigned i=0;i<count;++i){std::array<uint32_t,28>q{};for(auto&n:q)n=word(in);MaterialTypedDispatchCandidate v;v.created=q[0]!=0;v.value.numeric.typeAndMetadata=q[1];v.value.numeric.integer=q[2];std::copy_n(q.begin()+3,4,v.value.numeric.vector.begin());gate(q[10]<4096&&q[11]<1024,"Prepared variable text extent");v.value.text.resize(q[10]);v.name.resize(q[11]);in.read(v.value.text.data(),q[10]);in.read(v.name.data(),q[11]);if(q[8]){v.matrix.emplace();std::copy_n(q.begin()+12,16,v.matrix->begin());}v.matrixIdentity=q[9]!=0;m.parameters.values.push_back(std::move(v));}
    gate(bool(in),"Complete prepared original state");return p;
}
#ifdef SFM64_NATIVE_CLIENT_EQUALS
#include "material_float_input_client_native_fixture.h"
#endif
struct FloatInputProbeFactory:MaterialProxyFactoryCandidate {
    struct Instance:MaterialProxyInstanceCandidate {
        FloatInputProbeFactory&factory;MaterialProxyFloatInputCandidate input;MaterialProxyBindingCandidate drive,result;
        explicit Instance(FloatInputProbeFactory&f):factory(f){}
        bool initialize(const MaterialProxyContextCandidate&context,const SourceMaterialPatchNodeCandidate&declaration)override{
            drive=context.find("$drive");result=context.find("$out");const bool ok=drive.valid()&&result.valid()&&input.initialize(context,declaration,"input",factory.defaultBits);if(ok)factory.lastInput=input;return ok;
        }
        bool evaluate(MaterialProxyEvaluationCandidate&frame)override{
            const auto number=factory.frame++;
            if(number==1&&!frame.setVector(drive,{0x3fc00000,0xc0000000,0x40500000,0x40800000},2))return false;
            if(number==2&&!frame.setFloat(drive,0xbf400000))return false;
            auto value=input.read(frame);if(factory.observe&&!factory.observe(frame,input))return false;return value&&frame.setFloat(result,*value);
        }
    };
    uint32_t defaultBits=0,frame=0;
    MaterialProxyFloatInputCandidate lastInput;
    std::function<bool(MaterialProxyEvaluationCandidate&,const MaterialProxyFloatInputCandidate&)>observe;
    MaterialProxyInstanceCandidate*create(const std::string&name)override{return name=="FloatInputProbe"?new Instance(*this):nullptr;}
    void destroy(MaterialProxyInstanceCandidate*p)noexcept override{delete p;}
};
#ifndef SFM64_NATIVE_CLIENT_EQUALS
static int floatInputSafety();
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_CLIENT_EQUALS
    if(argc==2&&std::string(argv[1])=="--test")return floatInputSafety();
#endif
    gate(argc==5&&!std::ifstream(argv[4]).good(),"client-or-absent query prepared fresh-output");
    std::ifstream in(argv[2],std::ios::binary),prepared(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);gate(bool(in)&&bool(prepared)&&bool(out),"FloatInput IO");
#ifdef SFM64_NATIVE_CLIENT_EQUALS
    SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_FAILCRITICALERRORS);auto handler=AddVectoredExceptionHandler(1,&lifetimeFault);MaterialFloatInputClientNativeFixture client(argv[1]);unsigned old=0,control=0;_controlfp_s(&old,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"Owned FloatInput native x64");
#endif
    const auto mx=_mm_getcsr();_mm_setcsr(0x1f80);const auto count=word(in);gate(count>0&&count<10000&&word(prepared)==count,"FloatInput count");put(out,count);unsigned frames=0,refusals=0;
    for(unsigned index=0;index<count;++index){
        auto defaultBits=word(in);auto root=node(in);auto plan=readPrepared(prepared,root);auto*declarations=equalsDeclarations(plan);gate(declarations&&declarations->children.size()==1,"FloatInput declaration");
#ifdef SFM64_NATIVE_CLIENT_EQUALS
        currentCase=index;client.prepare(*plan.state);std::array<uint32_t,3>input{0x12345678,0,0xa5a5a5a5};const bool initialized=client.initializeInput(input,declarations->children.front(),defaultBits);
#else
        auto factory=std::make_shared<FloatInputProbeFactory>();factory->defaultBits=defaultBits;MaterialObjectParameterOptionsCandidate options;options.proxyFactory=factory;MaterialObjectParameterPlanCandidate own;gate(materialObjectParameterPrepareCandidate(own,"equals/case",0x10,&root,options),"FloatInput CPU prepare");std::ostringstream ownState,originalState;putValues(ownState,own.state->material);putValues(originalState,plan.state->material);gate(ownState.str()==originalState.str(),"Original/owned prepared identity");MaterialObjectStoreCandidate store;MaterialObjectStoreCandidate::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(root);uintptr_t id=0;gate(store.createMaterial("equals/case",source,id),"FloatInput create");const bool initialized=store.initializeParameters(id,options);
#endif
        put(out,index);put(out,initialized);if(!initialized)++refusals;
        if(initialized)for(unsigned frameNumber=0;frameNumber<3;++frameNumber){
#ifdef SFM64_NATIVE_CLIENT_EQUALS
            client.writes.clear();auto find=[&](const char*name){bool found=false;auto*p=MaterialEqualsClientNativeFixture::find(&client.material,nullptr,name,&found,true);gate(found,"FloatInput variable");return p;};
            if(frameNumber==1){const std::array<uint32_t,4>v{0x3fc00000,0xc0000000,0x40500000,0x40800000};MaterialEqualsClientNativeFixture::setVector(find("$drive"),nullptr,v.data(),2);}
            if(frameNumber==2)MaterialEqualsClientNativeFixture::setFloat(find("$drive"),nullptr,0xbf400000);
            MaterialEqualsClientNativeFixture::setFloat(find("$out"),nullptr,client.readInput(input));putWrites(out,client.writes);auto state=plan.state->material;state.parameters.values.clear();for(const auto&v:client.variables)state.parameters.values.push_back(v.value);putValues(out,state);
#else
            MaterialProxyEvaluationAttemptCandidate attempt;gate(store.evaluateProxies(id,{},&attempt)&&attempt.evaluated,"FloatInput evaluate");putWrites(out,attempt.writes);putValues(out,store.get(id)->parameterState->material);
#endif
            ++frames;
        }
#ifdef SFM64_NATIVE_CLIENT_EQUALS
        client.image.verify();gate(client.blocks.empty(),"FloatInput scratch cleanup");
#else
        gate(store.removeAndDelete(id),"FloatInput cleanup");
#endif
    }
    gate(in.peek()==std::char_traits<char>::eof()&&prepared.peek()==std::char_traits<char>::eof(),"FloatInput input tails");out.flush();gate(bool(out),"FloatInput complete output");_mm_setcsr(mx);
#ifdef SFM64_NATIVE_CLIENT_EQUALS
    client.image.verify();_controlfp_s(&control,old,_MCW_RC|_MCW_PC);RemoveVectoredExceptionHandler(handler);std::cout<<"native_scratch_allocations="<<client.allocations<<" releases="<<client.releases<<'\n';
#endif
    std::cout<<"PASS float_input_cases="<<count<<" frames="<<frames<<" refused="<<refusals<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
#ifndef SFM64_NATIVE_CLIENT_EQUALS
static int floatInputSafety(){
    using Store=MaterialObjectStoreCandidate;unsigned checks=0;auto check=[&](bool ok){if(!ok)throw std::runtime_error("FloatInput safety check="+std::to_string(checks));++checks;};
    auto root=[](const std::string&input){SourceMaterialPatchNodeCandidate r;r.key="UnlitTwoTexture";r.children={{"$out",3,"",0x3e800000},{"$drive",1,"[.25 -.5 .75 1]"}};SourceMaterialPatchNodeCandidate d;d.key="FloatInputProbe";d.children={{"input",1,input}};SourceMaterialPatchNodeCandidate proxies;proxies.key="Proxies";proxies.children.push_back(d);r.children.push_back(proxies);return r;};
    Store store;auto factory=std::make_shared<FloatInputProbeFactory>();Store::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(root("$drive[2]"));auto*original=source.get();auto originalRoot=*original;uintptr_t id=0;check(store.createMaterial("input/safety",source,id));MaterialObjectParameterOptionsCandidate options;options.proxyFactory=factory;check(store.initializeParameters(id,options));
    MaterialProxyFloatInputCandidate empty;factory->observe=[&](auto&frame,const auto&input){check(!empty.read(frame)&&input.read(frame)==std::optional<uint32_t>(0x3f400000));return true;};check(store.evaluateProxies(id));factory->observe={};
    auto stale=factory->lastInput;check(store.uncacheParameters(id)&&store.initializeParameters(id,options));factory->frame=0;factory->observe=[&](auto&frame,const auto&input){check(!stale.read(frame)&&input.read(frame)==std::optional<uint32_t>(0x3f400000));return true;};check(store.evaluateProxies(id));factory->observe={};
    auto snapshot=[&](){std::ostringstream s;putValues(s,store.get(id)->parameterState->material);return s.str();};const auto before=snapshot();auto*state=store.get(id)->parameterState.get();unsigned failures=0;bool success=false;
    for(int64_t n=0;n<2048;++n){const auto balance=heapNew-heapDelete;factory->frame=1;failAfter=n;const bool ok=store.evaluateProxies(id);failAfter=-1;if(ok){success=true;break;}++failures;check(heapNew-heapDelete==balance&&snapshot()==before&&store.get(id)->parameterState.get()==state&&!store.get(id)->evaluationPending&&materialObjectParameterTreeEqualCandidate(*original,originalRoot));}
    check(success&&failures>0);check(store.removeAndDelete(id));
    unsigned initFailures=0;bool initSuccess=false;
    for(int64_t n=0;n<2048;++n){
        Store s;auto f=std::make_shared<FloatInputProbeFactory>();Store::Source r=std::make_unique<SourceMaterialPatchNodeCandidate>(root("0.10000000000000000000000001"));auto originalCopy=*r;uintptr_t x=0;check(s.createMaterial("input/alloc",r,x));MaterialObjectParameterOptionsCandidate o;o.proxyFactory=f;const auto balance=heapNew-heapDelete;failAfter=n;const bool ok=s.initializeParameters(x,o);failAfter=-1;if(ok){initSuccess=true;check(s.removeAndDelete(x));break;}++initFailures;f->lastInput=MaterialProxyFloatInputCandidate{};check(heapNew-heapDelete==balance&&!s.get(x)->parameterState&&!s.get(x)->initializationPending&&materialObjectParameterTreeEqualCandidate(*s.get(x)->source,originalCopy));
    }
    check(initSuccess&&initFailures>0);
    for(unsigned kind:{4u,5u,6u,7u}){Store s;auto r=root("ignored");r.children.back().children.front().children.front().kind=kind;Store::Source p=std::make_unique<SourceMaterialPatchNodeCandidate>(r);uintptr_t x=0;check(s.createMaterial("input/refusal",p,x));check(!s.initializeParameters(x,options)&&!s.get(x)->parameterState);}
    Store longName;Store::Source r=std::make_unique<SourceMaterialPatchNodeCandidate>(root(std::string(255,'x')+"[0]"));uintptr_t x=0;check(longName.createMaterial("input/unsafe",r,x)&&!longName.initializeParameters(x,options));
    std::cout<<"PASS float_input_safety_checks="<<checks<<" evaluation_allocation_failures="<<failures<<" initialization_allocation_failures="<<initFailures<<'\n';return 0;
}
#endif
