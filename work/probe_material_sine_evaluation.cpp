#ifdef SFM64_NATIVE_CLIENT_EQUALS
#define SFM64_NATIVE_OBJECT_PARAMETERS
#endif
#define SFM64_OBJECT_PARAMETER_MAIN equalsPriorParameterRecorderMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#include "material_sine_proxy_candidate.h"
#ifdef SFM64_NATIVE_CLIENT_EQUALS
#include "material_sine_client_native_fixture.h"
#endif
static const SourceMaterialPatchNodeCandidate*equalsDeclarations(const MaterialObjectParameterPlanCandidate&plan){auto*p=materialProxySelectedNodeCandidate(plan.finalRoot,plan.state->material.selectedNode);gate(p!=nullptr,"Selected node");auto*r=sourceMaterialPatchFindConstCandidate(*p,"Proxies");return r?r:sourceMaterialPatchFindConstCandidate(plan.finalRoot,"Proxies");}
static void putWrites(std::ostream&out,const std::vector<MaterialProxyWriteCandidate>&writes){put(out,uint32_t(writes.size()));for(const auto&w:writes){put(out,w.index);put(out,w.operation);put(out,w.count);for(auto b:w.arguments)put(out,b);}}
static MaterialObjectParameterPlanCandidate readPrepared(std::istream&in,const SourceMaterialPatchNodeCandidate&root){
    MaterialObjectParameterPlanCandidate p;p.finalRoot=root;p.state=std::make_unique<MaterialObjectParameterStateCandidate>();auto&m=p.state->material;m.shader=text(in);auto count=word(in);gate(count>=13&&count<=255,"Prepared original variable count");
    for(unsigned i=0;i<count;++i){std::array<uint32_t,28>q{};for(auto&n:q)n=word(in);MaterialTypedDispatchCandidate v;v.created=q[0]!=0;v.value.numeric.typeAndMetadata=q[1];v.value.numeric.integer=q[2];std::copy_n(q.begin()+3,4,v.value.numeric.vector.begin());gate(q[10]<4096&&q[11]<1024,"Prepared variable text extent");v.value.text.resize(q[10]);v.name.resize(q[11]);in.read(v.value.text.data(),q[10]);in.read(v.name.data(),q[11]);if(q[8]){v.matrix.emplace();std::copy_n(q.begin()+12,16,v.matrix->begin());}v.matrixIdentity=q[9]!=0;m.parameters.values.push_back(std::move(v));}
    gate(bool(in),"Complete prepared original state");return p;
}
#ifndef SFM64_NATIVE_CLIENT_EQUALS
static int sineSafety();
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_CLIENT_EQUALS
    if(argc==2&&std::string(argv[1])=="--test-sine")return sineSafety();
#endif
    gate((argc==5||(argc==6&&std::string(argv[5])=="--sse2"))&&!std::ifstream(argv[4]).good(),"Private-client-or-absent query prepared-original-state fresh-output [--sse2]");std::ifstream in(argv[2],std::ios::binary),prepared(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);gate(bool(in)&&bool(prepared)&&bool(out),"Equals IO");
#ifdef SFM64_NATIVE_CLIENT_EQUALS
    SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_FAILCRITICALERRORS);
    MaterialSineClientNativeFixture client(argv[1]);client.word(0x854fc4,argc==6?1:0);auto handler=AddVectoredExceptionHandler(1,&lifetimeFault);unsigned old=0,temp=0;_controlfp_s(&old,0,0);_controlfp_s(&temp,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"Owned Equals runtime x64");
#endif
    const auto mx=_mm_getcsr();_mm_setcsr(0x1f80);auto count=word(in);gate(count&&count<10000&&word(prepared)==count,"Equals/prepared case count");put(out,count);unsigned frames=0,refused=0;
    for(unsigned caseIndex=0;caseIndex<count;++caseIndex){
        const std::array<uint32_t,3>times{word(in),word(in),word(in)};auto root=node(in);auto plan=readPrepared(prepared,root);auto*declarations=equalsDeclarations(plan);gate(declarations!=nullptr,"Proxy declarations required");
#ifdef SFM64_NATIVE_CLIENT_EQUALS
        currentCase=caseIndex;client.prepare(*plan.state);std::vector<void*>proxies;bool initialized=true;
        for(const auto&declaration:declarations->children){gate(declaration.key=="Equals"||declaration.key=="Sine","Pinned factory name");auto*p=client.createFunction(declaration.key);bool ok=client.initializeFunction(p,declaration);if(!ok){client.destroy(p);initialized=false;break;}gate(client.materialIdentity(p),"Original GetMaterial returns supplied material identity");proxies.push_back(p);}
#else
        MaterialObjectParameterOptionsCandidate options;options.proxyFactory=std::make_shared<MaterialFunctionProxyFactoryCandidate>();MaterialObjectParameterPlanCandidate own;gate(materialObjectParameterPrepareCandidate(own,"equals/case",0x10,&root,options),"Owned CPU state preparation");std::ostringstream actual,expected;putValues(actual,own.state->material);putValues(expected,plan.state->material);gate(actual.str()==expected.str(),"Owned initial state equals separately executed original materialsystem state");
        MaterialObjectStoreCandidate store;MaterialObjectStoreCandidate::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(root);uintptr_t id=0;gate(store.createMaterial("equals/case",source,id),"Owned Equals material");const bool initialized=store.initializeParameters(id,options);
#endif
        put(out,caseIndex);put(out,initialized);if(!initialized)++refused;
        if(initialized)for(unsigned frame=0;frame<3;++frame){
#ifdef SFM64_NATIVE_CLIENT_EQUALS
            client.writes.clear();for(auto*p:proxies){client.evaluateFunction(p,times[frame]);}putWrites(out,client.writes);auto value=plan.state->material;value.parameters.values.clear();for(const auto&v:client.variables)value.parameters.values.push_back(v.value);putValues(out,value);
            const auto&values=value.parameters.values;put(out,values[5].value.numeric.vector[0]);for(size_t k=0;k<3;++k)put(out,values[4].value.numeric.vector[k]);for(size_t k=0;k<3;++k){float a,b;auto x=values[4].value.numeric.vector[k],y=values[11].value.numeric.vector[k];std::memcpy(&a,&x,4);std::memcpy(&b,&y,4);volatile float product=a*b;float z=product;uint32_t productBits;std::memcpy(&productBits,&z,4);put(out,productBits);}
#else
            MaterialProxyEvaluationAttemptCandidate attempt;MaterialProxyEvaluationInputsCandidate inputs;inputs.currentTimeBits=times[frame];gate(store.evaluateProxies(id,{},&attempt,inputs)&&attempt.evaluated,"Owned evaluated frame");putWrites(out,attempt.writes);putValues(out,store.get(id)->parameterState->material);for(auto n:store.get(id)->forwarding.values)put(out,n);
#endif
            ++frames;
        }
#ifdef SFM64_NATIVE_CLIENT_EQUALS
        for(auto i=proxies.rbegin();i!=proxies.rend();++i)client.destroy(*i);client.image.verify();gate(client.blocks.empty(),"Native per-case proxy cleanup");
#else
        gate(store.removeAndDelete(id),"Owned Equals cleanup");
#endif
    }
    gate(in.peek()==std::char_traits<char>::eof()&&prepared.peek()==std::char_traits<char>::eof(),"Equals input/prepared tail");out.flush();gate(bool(out),"Complete Equals trace");_mm_setcsr(mx);
#ifdef SFM64_NATIVE_CLIENT_EQUALS
    client.image.verify();RemoveVectoredExceptionHandler(handler);_controlfp_s(&temp,old,_MCW_RC|_MCW_PC);std::cout<<"native_proxy_allocations="<<client.allocations<<" releases="<<client.releases<<'\n';
#endif
    std::cout<<"PASS sine_cases="<<count<<" frames="<<frames<<" init_refused="<<refused<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
#ifndef SFM64_NATIVE_CLIENT_EQUALS
extern "C" uint32_t materialTrigX87EnvironmentCandidate();
static int sineSafety(){
    using Store=MaterialObjectStoreCandidate;unsigned checks=0;auto check=[&](bool ok){if(!ok)throw std::runtime_error("Sine safety check="+std::to_string(checks));++checks;};
    SourceMaterialPatchNodeCandidate root;root.key="UnlitTwoTexture";root.children={{"$out",3,"",0x3e800000},{"$src",3,"",0x3f400000}};SourceMaterialPatchNodeCandidate e;e.key="Equals";e.children={{"resultVar",1,"$out"},{"srcVar1",1,"$src"}};SourceMaterialPatchNodeCandidate s;s.key="Sine";s.children={{"resultVar",1,"$out"}};SourceMaterialPatchNodeCandidate p;p.key="Proxies";p.children={e,s};root.children.push_back(p);
    Store store;Store::Source source=std::make_unique<SourceMaterialPatchNodeCandidate>(root);uintptr_t id=0;check(store.createMaterial("sine/safety",source,id));MaterialObjectParameterOptionsCandidate options;options.proxyFactory=std::make_shared<MaterialFunctionProxyFactoryCandidate>();check(store.initializeParameters(id,options));
    auto snapshot=[&](){std::ostringstream out;putValues(out,store.get(id)->parameterState->material);for(auto n:store.get(id)->forwarding.values)put(out,n);return out.str();};const auto before=snapshot();auto*state=store.get(id)->parameterState.get();MaterialProxyEvaluationAttemptCandidate attempt;
    check(!store.evaluateProxies(id,{},&attempt)&&!attempt.evaluated&&attempt.writes.empty()&&snapshot()==before&&store.get(id)->parameterState.get()==state);
    MaterialProxyEvaluationInputsCandidate context;context.currentTimeBits=0;
    check(!store.evaluateProxies(id,[](Store::Allocation a,size_t){return a!=Store::Allocation::evaluationCommit;},&attempt,context)&&snapshot()==before);
    unsigned failures=0;bool success=false;
    for(int64_t n=0;n<2048;++n){const auto balance=heapNew-heapDelete;failAfter=n;const bool ok=store.evaluateProxies(id,{},&attempt,context);failAfter=-1;if(ok){success=true;break;}++failures;check(heapNew-heapDelete==balance&&!attempt.evaluated&&attempt.writes.empty()&&snapshot()==before&&store.get(id)->parameterState.get()==state&&!store.get(id)->evaluationPending);}
    check(success&&failures>0&&attempt.evaluated&&attempt.writes.size()==2);auto initialized=snapshot();check(!store.evaluateProxies(id)&&snapshot()==initialized);
    check(attempt.writes.back().arguments[0]==0x3f000000);context.currentTimeBits=0x3e800000;check(store.evaluateProxies(id,{},&attempt,context)&&attempt.writes.back().arguments[0]==0x3f800000);
    const auto environment=materialTrigX87EnvironmentCandidate();volatile double large=materialSineDoubleCandidate(1e80);(void)large;check(materialTrigX87EnvironmentCandidate()==environment);
    check(store.uncacheParameters(id)&&!store.evaluateProxies(id,{},nullptr,context)&&store.initializeParameters(id,options)&&store.removeAndDelete(id));
    for(const auto&name:{"$alpha","$out[4]","$basetexturetransform"}){Store a;auto r=root;r.children.back().children={s};r.children.back().children.front().children.front().text=name;Store::Source src=std::make_unique<SourceMaterialPatchNodeCandidate>(r);uintptr_t x=0;check(a.createMaterial("sine/refusal",src,x)&&!a.initializeParameters(x,options)&&!a.get(x)->parameterState);}
    std::cout<<"PASS sine_safety_checks="<<checks<<" actual_allocation_failures="<<failures<<'\n';return 0;
}
#endif
